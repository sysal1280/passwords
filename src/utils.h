#ifndef UTILS_H
#define UTILS_H

#include "settings.h"
#include <QMessageBox>
#include <QApplication>
#include <QResource>
#include <QStandardPaths>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <QLabel>
#include <QStatusBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

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
    QLabel *debugWarning = new QLabel("📛 WARNING: Debug version — Do NOT use for real passwords 📛");
    debugWarning->setStyleSheet("color: red; font-weight: bold;");
    statusBar->addPermanentWidget(debugWarning);

    // Repeating messagebox every 5 minutes
    QTimer *debugTimer = new QTimer(parent);
    QObject::connect(debugTimer, &QTimer::timeout, parent, []() {
        QMessageBox::critical(
            nullptr,
            QApplication::applicationName(),
            "This is a DEBUG build.\n\n"
            "It is not safe for production use. It is intended for testing purposes only. "
            "Do not store real passwords or other proper data in this database.\n\nIf this program has been installed for you, uninstall it immediately."
            );
    });
    debugTimer->start(5 * 60 * 1000); // 5 minutes
#else
    Q_UNUSED(parent)
    Q_UNUSED(statusBar)
#endif
}

inline void checkHelpReachable(std::function<void(bool)> callback,
                               QObject* parent = nullptr)
{
    auto manager = new QNetworkAccessManager(parent);
    QNetworkRequest request(QUrl("https://sysal1280.github.io/passwords/"));
    QNetworkReply* reply = manager->head(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, callback]() {
        bool ok = (reply->error() == QNetworkReply::NoError);
        callback(ok);
        reply->deleteLater();
    });
}

#endif // UTILS_H
