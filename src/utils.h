#ifndef UTILS_H
#define UTILS_H

#include "settings.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>
#include <QResource>

inline QString loadWordlistResource(const QWidget *parent,
                                    const char *caller)
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



#endif // UTILS_H
