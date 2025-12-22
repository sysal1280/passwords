#ifndef UTILS_H
#define UTILS_H

#include "settings.h"
#include <QMessageBox>
#include <QApplication>
#include <QResource>
#include <QStandardPaths>
#include <QDialogButtonBox>
#include <QPushButton>

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

#endif // UTILS_H
