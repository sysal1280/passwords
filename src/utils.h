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


#ifndef UTILS_H
#define UTILS_H

#include "settings.h"
#include "constants.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QResource>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <QProgressDialog>
#include <QElapsedTimer>


inline QString loadWordlistResource(const QWidget *parent, const char *caller)
{
    Settings settings;
    QString configFile = settings.configFilePath();
    QString configDir  = QFileInfo(configFile).absolutePath();

    QString wordListPath = QDir(configDir).filePath("wordlist.rcc");

    if (!QFile::exists(wordListPath)) {
        qCritical().noquote() << caller << "Missing resource file:" << wordListPath;
        QMessageBox::critical(const_cast<QWidget*>(parent),
                              QApplication::applicationName(),
                              QString("Missing resource file:\n%1").arg(wordListPath));
        return QString();
    }

    if (!QResource::registerResource(wordListPath)) {
        qCritical().noquote() << caller << "Failed to register resource:" << wordListPath;
        QMessageBox::critical(const_cast<QWidget*>(parent),
                              QApplication::applicationName(),
                              QString("Failed to register resource:\n%1").arg(wordListPath));
        return QString();
    }

    qInfo().noquote() << "Loaded wordlist.rcc file.";
    return wordListPath;
}

inline QString appKeyFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    const QString path = dir + "/appkey";
    return QFile::exists(path) ? path : QString();
}

inline bool hasHelp(QPushButton *helpButton = nullptr)
{
    const QString appDir = QCoreApplication::applicationDirPath();

    const bool ok =
        (QFile::exists(appDir + "/pwdhlp.exe") ||
         QFile::exists(appDir + "/pwdhlp"))
        && QFile::exists(appDir + "/pwdhlp.rcc");

    if (!ok && helpButton)
        helpButton->setEnabled(false);

    return ok;
}

inline void setupDebugWarnings(QWidget *parent, QStatusBar *statusBar)
{
#ifdef APP_DEBUG_BUILD
    // Permanent status bar warning
    QWidget *container = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4); // small gap between icon and text

    QLabel *iconLabel = new QLabel;
    QIcon beetleIcon(":/menus/glyphs/beetle_color.svg");
    iconLabel->setPixmap(beetleIcon.pixmap(16, 16));

    QLabel *textLabel = new QLabel(Passwords::debugWarningSB);
    textLabel->setStyleSheet("color: red; font-weight: bold;");

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);

    statusBar->addPermanentWidget(container);

    // Repeating messagebox every 5 minutes
    QTimer *debugTimer = new QTimer(parent);

    // Extract the lambda so we can call it manually
    auto showDebugMessage = []() {
        QIcon beetleIcon(":/menus/glyphs/beetle_color.svg");
        QPixmap beetlePixmap = beetleIcon.pixmap(48, 48);

        QMessageBox msgBox;
        msgBox.setWindowTitle(QApplication::applicationName());
        msgBox.setText(Passwords::debugWarningMB);
        msgBox.setIconPixmap(beetlePixmap);
        msgBox.exec();
    };

    QObject::connect(debugTimer, &QTimer::timeout, parent, showDebugMessage);
    QTimer::singleShot(10000, parent, showDebugMessage);

    // Start the 5‑minute cycle
    debugTimer->start(5 * 60 * 1000); // 5 minutes

#else
    Q_UNUSED(parent)
    Q_UNUSED(statusBar)
#endif
}

inline QString getHelpBaseUrl(const QString& page = QString())
{
    QString lang = QLocale::system().name().left(2);

    // Supported languages
    static const QSet<QString> supported = { "en", "fr", "de" };
    if (!supported.contains(lang))
        lang = "en";

    // Base: https://.../passwords/en/
    QString base = QString("%1%2/").arg(Passwords::HelpBaseUrl, lang);

    // If a page is provided, append it
    if (!page.isEmpty()) {
        // Ensure no accidental leading slash
        QString cleanPage = page;
        if (cleanPage.startsWith('/'))
            cleanPage.remove(0, 1);

        base += cleanPage;
    }

    return base;
}

inline void checkHelpReachable(std::function<void(bool)> callback,
                               QObject* parent = nullptr)
{
    auto manager = new QNetworkAccessManager(parent);
    auto request = QNetworkRequest(QUrl(getHelpBaseUrl()));

    QNetworkReply* reply = manager->head(request);

    // --- Timeout timer ---
    QTimer* timer = new QTimer(reply);   // parented to reply for auto cleanup
    timer->setSingleShot(true);
    timer->start(3000);                  // 3 seconds

    QObject::connect(timer, &QTimer::timeout, reply, [reply, callback]() {
        // Timeout reached → abort the request
        reply->abort();
        callback(false);
        reply->deleteLater();
    });

    // --- Normal completion ---
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, callback, timer]() {
        if (timer->isActive())
            timer->stop();   // prevent timeout firing after success

        bool ok = (reply->error() == QNetworkReply::NoError);
        callback(ok);
        reply->deleteLater();
    });
}

inline void manageBackups(const QString &directory, int maxFiles, QWidget *parent = nullptr)
{
    if (maxFiles <= 0)
        return;

    QDir dir(directory);
    if (!dir.exists())
        return;

    QFileInfoList files = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time | QDir::Reversed   // Oldest first
        );

    if (files.size() <= maxFiles)
        return;

    int toDelete = files.size() - maxFiles;

    QProgressDialog *progress = nullptr;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < toDelete; ++i) {

        // Show progress dialog only if operation takes > 1 second
        if (!progress && parent && timer.elapsed() > 1000) {
            progress = new QProgressDialog(
                QObject::tr("Cleaning old backups…"),
                QObject::tr("Cancel"),
                0,
                toDelete,
                parent
                );
            progress->setWindowModality(Qt::WindowModal);
            progress->setValue(i);
            progress->show();
        }

        if (progress) {
            progress->setValue(i);
            QCoreApplication::processEvents();

            if (progress->wasCanceled())
                break;
        }

        QFile::remove(files[i].absoluteFilePath());
    }

    if (progress) {
        progress->setValue(toDelete);
        progress->deleteLater();
    }
}

#endif // UTILS_H
