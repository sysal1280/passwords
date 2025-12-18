/*
 * passwords - A simple password manager
 *
 * Copyright (C) 2025  Adam.Lanzafame <sysal@tuta.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "gpgCheck.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QProcess>
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>
#include "settings.h"
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QFileInfo>
#include "DataObfuscator.h"
#include <QApplication>

bool isToolAvailable(const QString &toolName)
{
    // Use QStandardPaths::findExecutable to find the tool
    QString executablePath = QStandardPaths::findExecutable(toolName);

    // If the executable is found, log the path
    if (!executablePath.isEmpty()) {
        return true;
    } else {
        return false;
    }
}

// Worker function: runs in background
// used by checkGpgKeys.
static QStringList checkKeysWithGpg(const QStringList &keys) {
    QStringList invalidKeys;

    for (const QString &keyId : keys) {
        QProcess gpg;
        gpg.start("gpg", {"--list-keys", keyId});
        if (!gpg.waitForFinished(5000)) {
            qDebug() << "gpg timed out for key:" << keyId;
            invalidKeys << keyId;
            continue;
        }

        if (gpg.exitStatus() != QProcess::NormalExit || gpg.exitCode() != 0) {
            qDebug() << "gpg failed for key:" << keyId << gpg.errorString();
            invalidKeys << keyId;
            continue;
        }

        QByteArray output = gpg.readAllStandardOutput();
        if (!output.contains(keyId.toUtf8())) {
            invalidKeys << keyId;
        }
    }

    return invalidKeys;
}

void checkGpgKeys(QWidget* parent)
{
    Settings settings;
    QString dbFile = settings.getDefaultDbPath(parent);
    if (!QFileInfo::exists(dbFile))
    {
        qDebug() << "skipping GPG keys check, no database.";
        return;
    }

    const QString connNameRead = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QStringList keys;

    // Read keys
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connNameRead);
        db.setDatabaseName(dbFile);

        if (!db.open()) {
            qWarning() << "DB open failed:" << db.lastError().text();
            QSqlDatabase::removeDatabase(connNameRead);
            return;
        }


        qDebug() << qApp->property("appKey").toByteArray();

        QSqlQuery select(db);
        if (!select.exec("SELECT key FROM keys")) {
            qWarning() << "gpgCheck Error:" << select.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase(connNameRead);
            return;
        }

        while (select.next())
        {
            keys << DataObfuscator::deobfuscate(select.value(0).toString(),qApp->property("appKey").toByteArray());
            qDebug() << DataObfuscator::deobfuscate(select.value(0).toString(),qApp->property("appKey").toByteArray());
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connNameRead);

    // Async GPG check
    auto *watcher = new QFutureWatcher<QStringList>(parent);
    QObject::connect(watcher, &QFutureWatcher<QStringList>::finished, parent,
                     [parent, watcher, &settings]() {
                         QStringList invalidKeys = watcher->result();

                         if (!invalidKeys.isEmpty()) {
                             const QString connNameWrite = QUuid::createUuid().toString();
                             {
                                 QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connNameWrite);
                                 db.setDatabaseName(settings.getDefaultDbPath(parent));
                                 if (db.open()) {
                                     for (const auto &keyId : std::as_const(invalidKeys)) {
                                         QSqlQuery remove(db);
                                         remove.prepare("DELETE FROM keys WHERE key = :key");
                                         remove.bindValue(":key", DataObfuscator::obfuscate(keyId,qApp->property("appKey").toByteArray()));
                                         if (!remove.exec())
                                             qWarning() << "Failed to remove key:" << keyId << remove.lastError().text();
                                     }
                                     db.close();
                                 }
                             }
                             QSqlDatabase::removeDatabase(connNameWrite);
                         }

                         watcher->deleteLater();
                     });

    watcher->setFuture(QtConcurrent::run(checkKeysWithGpg, keys));
}

bool isStrong(const QString &str)
{
    if (str.size() < 10)
        return false;

    bool hasLower = false;
    bool hasUpper = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (const QChar &c : str) {
        if (c.isLower())
            hasLower = true;
        else if (c.isUpper())
            hasUpper = true;
        else if (c.isDigit())
            hasDigit = true;
        else
            hasSpecial = true;

        // Early exit: all categories found
        if (hasLower && hasUpper && hasDigit && hasSpecial)
            return true;
    }

    return false;
}

bool warnAndContinue()
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Weak Password");
    msgBox.setText(
        "Your password can be easily cracked in a short time.\n\n"
        "A safe password should be at least 10 characters long and contain:\n"
        "• Lowercase letters\n"
        "• Uppercase letters\n"
        "• Digits\n"
        "• Special characters\n\n"
        "Click OK to ignore this advice and continue."
        );
    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    if (msgBox.exec() == QMessageBox::Cancel) {
        return false;
    } else
    {
        return true;
    }
}

bool hasUltimateTrust(const QString &keyId)
{
    QProcess gpg;
    gpg.start("gpg", {"--list-keys", "--with-colons", keyId});
    if (!gpg.waitForFinished(5000))
        return false;

    if (gpg.exitStatus() != QProcess::NormalExit || gpg.exitCode() != 0)
        return false;

    const QString output = gpg.readAllStandardOutput();
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("pub:")) {
            const QStringList fields = line.split(':');
            if (fields.size() > 1 && fields.at(1) == "u")
                return true;
        }
    }
    return false;
}


