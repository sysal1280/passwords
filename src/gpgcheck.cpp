/*
 * passwords - A GnuPG based password manager
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


#include "gpgcheck.h"
#include "dataobfuscator.h"
#include "settings.h"
#include "utils.h"
#include "mainwindow.h"
#include "constants.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QClipboard>
#include <QDesktopServices>
#include <QRegularExpression>


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
            qDebug().noquote() << Q_FUNC_INFO << "gpg timed out for key:" << keyId;
            invalidKeys << keyId;
            continue;
        }

        if (gpg.exitStatus() != QProcess::NormalExit || gpg.exitCode() != 0) {
            qDebug().noquote() << Q_FUNC_INFO  << "gpg failed for key:" << keyId << gpg.errorString();
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
    const QString dbFile = settings.getDefaultDbPath(parent);
    if (!QFileInfo::exists(dbFile)) {
        qWarning().noquote() << Q_FUNC_INFO
                             << "skipping GPG keys check. There is no database.";
        return;
    }

    QStringList keys;

    // Read keys (separate scope so db and query die before removeDatabase)
    const QString connNameRead = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connNameRead);
        db.setDatabaseName(dbFile);

        if (!db.open()) {
            qCritical().noquote() << "DB open failed:" << db.lastError().text();
            QSqlDatabase::removeDatabase(connNameRead);
            return;
        }

        QSqlQuery select(db);
        if (!select.exec("SELECT key FROM keys")) {
            qWarning().noquote() << "gpgCheck Error:" << select.lastError().text();
            db.close();
            // db and select will be destroyed at end of scope
            QSqlDatabase::removeDatabase(connNameRead);
            return;
        }

        const QByteArray appKey = qApp->property("appKey").toByteArray();

        while (select.next()) {
            const QString obfuscated = select.value(0).toString();
            const QString key = DataObfuscator::deobfuscate(obfuscated, appKey);
            keys << key;
            qInfo().noquote() << "Checking Key:" << key;
        }

        db.close();
    }
    // All objects using this connection are now destroyed
    QSqlDatabase::removeDatabase(connNameRead);

#ifdef Q_OS_WIN
    if (!keys.isEmpty()) {
        warmupGpg(keys.first(), parent);
    }
#endif

    // Async GPG check
    auto *watcher = new QFutureWatcher<QStringList>(parent);

    // We only need the DB path in the async continuation, not Settings or parent
    const QString dbFileForWrite = dbFile;

    QObject::connect(
        watcher,
        &QFutureWatcher<QStringList>::finished,
        watcher, // use watcher as the receiver to avoid depending on 'parent'
        [watcher, dbFileForWrite]() {
            const QStringList invalidKeys = watcher->result();

            if (!invalidKeys.isEmpty()) {
                const QString connNameWrite = QUuid::createUuid().toString(QUuid::WithoutBraces);
                {
                    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connNameWrite);
                    db.setDatabaseName(dbFileForWrite);
                    if (db.open()) {
                        const QByteArray appKey = qApp->property("appKey").toByteArray();
                        for (const auto &keyId : std::as_const(invalidKeys)) {
                            QSqlQuery remove(db);
                            remove.prepare("DELETE FROM keys WHERE key = :key");
                            remove.bindValue(
                                ":key",
                                DataObfuscator::obfuscate(keyId, appKey)
                                );
                            if (!remove.exec()) {
                                qWarning().noquote() << Q_FUNC_INFO
                                                     << "Failed to remove key:" << keyId
                                                     << remove.lastError().text();
                            }
                        }
                        db.close();
                    }
                }
                // db and queries destroyed before removing the connection
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

bool warmupGpg(const QString &recipientKey, QWidget *parent)
{
    if (recipientKey.isEmpty())
        return false;

    QProcess warmup;

    // Feed input once the process starts
    QObject::connect(&warmup, &QProcess::started, [&warmup]() {
        warmup.write("warmup\n");
        warmup.closeWriteChannel();
    });

    bool ok = false;
    QObject::connect(&warmup, &QProcess::finished,
                     [&](int exitCode, QProcess::ExitStatus status) {
                         ok = (status == QProcess::NormalExit && exitCode == 0);
                     });

    warmup.start("gpg", {
                            "--batch",
                            "--yes",
                            "--pinentry-mode", "loopback",
                            "--encrypt",
                            "--recipient", recipientKey,
                            "--output", "NUL"
                        });

    // Wait while keeping UI responsive
    QEventLoop loop;
    QObject::connect(&warmup, &QProcess::finished, &loop, &QEventLoop::quit);

    // Timeout protection
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (warmup.state() == QProcess::Running)
            warmup.kill();
        loop.quit();
    });
    timer.start(12000);

    loop.exec(); // keeps splash screen responsive

    if (!ok) {
        qWarning().noquote() << "GPG warm-up failed:"
                             << warmup.readAllStandardError();
    }

    return ok;
}

void showGpgKeyListDialog(GpgKeyType type, const QString &userName, QWidget *parent)
{
    // --- Build dialog dynamically ---
    QDialog* dlg = new QDialog(parent);
    dlg->setWindowTitle(type == GpgKeyType::Public
                            ? QObject::tr("GPG Public Keys")
                            : QObject::tr("GPG Secret Keys"));
    dlg->resize(650, 550);

    QVBoxLayout* layout = new QVBoxLayout(dlg);

    // --- Add label at the top ---
    QString keyTypeText = (type == GpgKeyType::Public)
                              ? QObject::tr("public")
                              : QObject::tr("private");

    QLabel* infoLabel = new QLabel(
        QObject::tr("These are the installed %1 keys for %2\nWhen linking keys, use the Key ID or Fingerprint.")
            .arg(keyTypeText, userName),
        dlg
        );
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // --- Create text edit inside dialog ---
    QTextEdit* textEdit = new QTextEdit(dlg);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    // --- Button box ---
    QDialogButtonBox* buttonBox = new QDialogButtonBox(dlg);
    QPushButton* closeBtn = buttonBox->addButton(QDialogButtonBox::Close);
    QPushButton* copyBtn  = buttonBox->addButton(QObject::tr("Copy"), QDialogButtonBox::ActionRole);
    QPushButton* helpBtn  = buttonBox->addButton(QDialogButtonBox::Help);

    layout->addWidget(buttonBox);

    // --- Button actions ---
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    QObject::connect(copyBtn, &QPushButton::clicked, dlg, [textEdit]() {
        QApplication::clipboard()->setText(textEdit->toPlainText());
    });

    QObject::connect(helpBtn,
                     &QPushButton::clicked,
                     dlg,
                     [dlg]() {

                         checkHelpReachable([dlg](bool reachable) {
                             if (reachable) {
                                 // Open the online help page for linking keys
                                 const QUrl url(Passwords::HelpBaseUrl + QStringLiteral("keys-listing"));
                                 QDesktopServices::openUrl(url);
                             } else {
                                 // Fallback to helper process
                                 MainWindow *mw = qobject_cast<MainWindow*>(dlg->parentWidget());
                                 if (!mw) {
                                     QMessageBox::warning(
                                         dlg,
                                         QObject::tr("Help Error"),
                                         QObject::tr("Help system unavailable: parent window is not MainWindow.")
                                         );
                                     return;
                                 }

                                 mw->launchHelperProcess(QStringLiteral("keys-listing"));
                             }
                         });
                     });



    // --- Create process ---
    QProcess* proc = new QProcess(dlg);

    QObject::connect(proc, &QProcess::readyReadStandardOutput, dlg, [textEdit, proc]() {
        textEdit->append(QString::fromUtf8(proc->readAllStandardOutput()));
    });

    QObject::connect(proc, &QProcess::readyReadStandardError, dlg, [textEdit, proc]() {
        textEdit->append("<span style='color:red'>" +
                         QString::fromUtf8(proc->readAllStandardError()) +
                         "</span>");
    });

    // --- When finished, prettify output ---
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     dlg, [textEdit, type](int, QProcess::ExitStatus) {

                         QString raw = textEdit->toPlainText();
                         textEdit->clear();

                         QStringList lines = raw.split('\n');
                         QString out;

                         QString keyId;
                         QString fingerprint;
                         QString created;

                         const QString primaryType =
                             (type == GpgKeyType::Public) ? "pub" : "sec";

                         for (const QString& line : lines) {
                             QStringList p = line.split(':');
                             if (p.size() < 10)
                                 continue;

                             const QString recordType = p[0];

                             if (recordType == primaryType) {
                                 keyId.clear();
                                 fingerprint.clear();
                                 created.clear();

                                 keyId = p[4];

                                 if (!p[5].isEmpty()) {
                                     created = QDateTime::fromSecsSinceEpoch(
                                                   p[5].toLongLong())
                                                   .toString("yyyy-MM-dd");
                                 }
                             }
                             else if (recordType == "fpr") {
                                 if (fingerprint.isEmpty())
                                     fingerprint = p[9].trimmed();
                             }
                             else if (recordType == "uid") {
                                 if (keyId.isEmpty())
                                     continue;

                                 QString uid = p[9];
                                 QString user;
                                 QString email;

                                 QRegularExpression re(R"(^(.*)\s+<(.*)>)");
                                 auto m = re.match(uid);
                                 if (m.hasMatch()) {
                                     user = m.captured(1).trimmed();
                                     email = m.captured(2).trimmed();
                                 } else {
                                     user = uid.trimmed();
                                 }

                                 out += QString(
                                            "User:         %1\n"
                                            "Email:        %2\n"
                                            "Key ID:       %3\n"
                                            "Fingerprint:  %4\n"
                                            "Created:      %5\n"
                                            "--------------------------------\n"
                                            ).arg(user,
                                                 email,
                                                 keyId,
                                                 fingerprint.isEmpty() ? "(none)" : fingerprint,
                                                 created);
                             }
                         }

                         textEdit->setPlainText(out);
                     });

    // --- Start GPG asynchronously ---
    QStringList args;
    if (type == GpgKeyType::Public)
        args << "--list-public-keys" << "--with-colons";
    else
        args << "--list-secret-keys" << "--with-colons";

    proc->start("gpg", args);

    dlg->show();
}

void createGpgEncryptionKeyAsync(
    const QString &name,
    QWidget *parent,
    std::function<void(bool)> done)
{
    if (name.isEmpty()) {
        QMessageBox::warning(parent, "Missing Information",
                             "A name is required to create a GPG key.");
        done(false);
        return;
    }

    struct State {
        QString userId;
        QString fingerprint;
        QWidget *parent;
        std::function<void(bool)> done;
        QProcess *proc = nullptr;
    };

    State *s = new State{ name, "", parent, done, new QProcess(parent) };

    auto fail = [s](const QString &msg){
        QMessageBox::critical(s->parent, "GPG Error", msg);
        s->done(false);
        s->proc->deleteLater();
        delete s;
    };

    auto step3 = [s, fail]() {
        // STEP 3 — Add encryption subkey
        QObject::disconnect(s->proc, nullptr, nullptr, nullptr);

        QObject::connect(s->proc, &QProcess::finished, [s, fail](int exitCode){
            if (exitCode != 0) {
                fail("Failed to add encryption subkey:\n\n" +
                     s->proc->readAllStandardError());
                return;
            }

            s->done(true);
            s->proc->deleteLater();
            delete s;
        });

        s->proc->setProgram("gpg");
        s->proc->setArguments({
            "--batch",
            "--quick-add-key",
            s->fingerprint,
            "cv25519",
            "encrypt",
            "0"
        });
        s->proc->start();
    };

    auto step2 = [s, fail, step3]() {
        // STEP 2 — Extract fingerprint
        QObject::disconnect(s->proc, nullptr, nullptr, nullptr);

        QObject::connect(s->proc, &QProcess::finished, [s, fail, step3](int exitCode){
            if (exitCode != 0) {
                fail("Failed to read fingerprint.");
                return;
            }

            QString output = s->proc->readAllStandardOutput();
            for (const QString &line : output.split('\n')) {
                if (line.startsWith("fpr:")) {
                    QStringList parts = line.split(':');
                    if (parts.size() > 9) {
                        s->fingerprint = parts[9];
                        break;
                    }
                }
            }

            if (s->fingerprint.isEmpty()) {
                fail("Could not extract fingerprint.");
                return;
            }

            step3();
        });

        s->proc->setProgram("gpg");
        s->proc->setArguments({
            "--with-colons",
            "--fingerprint",
            s->userId
        });
        s->proc->start();
    };

    auto step1 = [s, fail, step2]() {
        // STEP 1 — Create primary key
        QObject::connect(s->proc, &QProcess::finished, [s, fail, step2](int exitCode){
            if (exitCode != 0) {
                fail("Failed to create primary key:\n\n" +
                     s->proc->readAllStandardError());
                return;
            }
            step2();
        });

        s->proc->setProgram("gpg");
        s->proc->setArguments({
            "--batch",
            "--quick-generate-key",
            s->userId,
            "ed25519",
            "sign",
            "0"
        });
        s->proc->start();
    };

    step1();
}
