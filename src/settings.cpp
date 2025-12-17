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


#include "settings.h"
#include <QFileDialog>
#include <QFile>
#include <QApplication>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>
#include <QInputDialog>

Settings::Settings()
{
    configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");

    configFile = configDir + "/" + CONFIG_FILENAME;
    QString wlFile = configDir + "/" + WORDLIST_FILENAME;

    auto ensureFile = [](const QString &targetPath,
                         const QString &resourcePath,
                         const QString &description)
    {
        if (!QFile::exists(targetPath)) {
            qDebug() << "Copying" << description << "file";
            if (!QFile::copy(resourcePath, targetPath)) {
                qWarning().noquote() << "Could not copy" << description << "to" << targetPath;
            } else {
                QFile::setPermissions(targetPath,
                                      QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            }
        }
    };

    ensureFile(configFile, ":/files/passwords.conf", "template config");
    ensureFile(wlFile, ":/files/wordlist.rc", "wordlist resource");

    settings = new QSettings(configFile, QSettings::IniFormat);
}


QString Settings::configFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(dir).mkpath(".");
    qDebug() << Q_FUNC_INFO <<  dir + "/" + CONFIG_FILENAME;
    return dir + "/" + CONFIG_FILENAME;
}

void Settings::setLastUsedFile(const QString &filePath)
{

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("General");
    settings.setValue("LastDatabase", filePath);
    settings.endGroup();
    settings.sync();
    qDebug() << "Saving"<<filePath<<"as last database";
}


QString Settings::getLastUsedFile()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("General");
    QString val = settings.value("LastDatabase", "").toString();
    settings.endGroup();

    return val;
}

QString Settings::getWordListFile()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    QString val = QString(":/wordlist/%1").arg(settings.value("WordList", "").toString());
    settings.endGroup();

    return val;
}


QString Settings::getPathSeparator()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    QString val = settings.value("PathSeparator", " / ").toString();
    settings.endGroup();

    return val;
}

bool Settings::getBackupDatabase()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("General");
    QString val = settings.value("BackupDatabase", "yes").toString().trimmed().toLower();
    settings.endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getCloseHelpServer()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Help");
    QString val = settings.value("CloseServer", "yes").toString().trimmed().toLower();
    settings.endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getDragDropPrompt()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    QString val = settings.value("DragDropPrompt", "yes").toString().trimmed().toLower();
    settings.endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getKillGpgAgent()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    QString val = settings.value("KillGPGAgent", "no").toString().trimmed().toLower();
    settings.endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

int Settings::getGeneratedPasswordLength()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant here
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    int val = settings.value("GeneratedPasswordLength", 2).toInt();
    settings.endGroup();
    return val;
}

QLineEdit::EchoMode Settings::getEchoMode()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("General");
    int val = settings.value("EchoMode", QLineEdit::Password).toInt();
    settings.endGroup();
    return static_cast<QLineEdit::EchoMode>(val);
}


int Settings::getHelpPort()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant here
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Help");
    int val = settings.value("Port", 1280).toInt();
    settings.endGroup();
    return val;
}

int Settings::getMaxRecentResults()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant here
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Search");
    int val = settings.value("MaxRecentResults", 15).toInt();
    settings.endGroup();
    return val;
}

int Settings::getMaxPopularResults()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant here
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Search");
    int val = settings.value("MaxPopularResults", 15).toInt();
    settings.endGroup();
    return val;
}

int Settings::getAutoCloseSeconds()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;   // use the constant here
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("Passwords");
    int val = settings.value("AutoClose", 0).toInt();
    settings.endGroup();
    return val;
}

bool Settings::getLoginPreference()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("General");
    QString val = settings.value("RequireChallenge", "yes").toString().trimmed().toLower();
    settings.endGroup();
    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getAskClose()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);
    settings.beginGroup("General");
    QString val = settings.value("AskBeforeClosing", "yes").toString().trimmed().toLower();
    settings.endGroup();
    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

void Settings::setGeneratedPasswordLength(int i)
{
    // build config dir
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists())
        dir.mkpath(".");

    // use the constant filename
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("Passwords");
    settings.setValue("GeneratedPasswordLength", i);
    settings.endGroup();
}

QString Settings::getDefaultDbPath(QWidget* parent)
{
    // 1. Last used file
    if (QFile::exists(getLastUsedFile())) {
        return getLastUsedFile();
    }

    // 2. Local user data path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(configDir).mkpath(".");
    QString localPath = QDir(configDir).filePath("passwords");

    qDebug() << "Looking for database at" << localPath;
    if (QFile::exists(localPath)) {
        return localPath;
    }

    // 3. Global system path (OS-specific)
#ifdef Q_OS_WIN
    QString globalPath = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                             .filePath("Passwords/passwords");
#elif defined(Q_OS_MAC)
    QString globalPath = "/Library/Application Support/Passwords/passwords";
#else
    QString globalPath = "/var/lib/password/passwords";
#endif

    qDebug() << "Looking for global database at" << globalPath;
    if (QFile::exists(globalPath)) {
        return globalPath;
    }

    // 4. Portable mode: next to the executable
    QString portablePath = QDir(QCoreApplication::applicationDirPath()).filePath("passwords");
    if (QFile::exists(portablePath)) {
        return portablePath;
    }

    return "";
}

// Save MainWindow geometry/state
void Settings::saveMainWindowState(QMainWindow *window)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", window->saveGeometry());
    settings.setValue("state", window->saveState());
    settings.endGroup();
}

// Restore MainWindow geometry/state
void Settings::restoreMainWindowState(QMainWindow *window)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("MainWindow");
    window->restoreGeometry(settings.value("geometry").toByteArray());
    window->restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}

// Save splitter state
void Settings::saveSplitterState(QSplitter *splitter, const QString &name)
{
    if (!splitter)
    {
        return;
    }

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    QByteArray state = splitter->saveState();
    qDebug() << "Saving splitter" << name << "state size:" << state.size();

    settings.beginGroup("Splitters");
    settings.setValue(name, state);
    settings.endGroup();
    settings.sync();
}

// Restore splitter state
void Settings::restoreSplitterState(QSplitter *splitter, const QString &name)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFile = configDir + "/" + CONFIG_FILENAME;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup("Splitters");
    splitter->restoreState(settings.value(name).toByteArray());
    settings.endGroup();
}

bool Settings::createUserDesktopFile()
{

#if !defined(Q_OS_LINUX)
    return false; // only do this on Linux
#endif

    QFileInfo exeInfo(QCoreApplication::applicationFilePath());
    QString exePath = exeInfo.absoluteFilePath();

    // "installed" dirs we consider valid
    QStringList standardDirs = {
        "/usr/bin",
        "/usr/local/bin",
        "/bin",
        "/sbin",
        QDir::homePath() + "/.local/bin",
        QDir::homePath() + "/bin"
    };

    bool installed = false;
    for (const QString &dir : standardDirs) {
        if (exePath.startsWith(dir)) {
            installed = true;
            break;
        }
    }
    if (!installed)
        return false;

    // target directory for user .desktop files
    QString localAppsDir = QDir::homePath() + "/.local/share/applications";
    QDir().mkpath(localAppsDir);

    QString desktopFilePath = localAppsDir + "/passwords.desktop";

    if (QFile::exists(desktopFilePath)) {
        return false;
    }

    // load template from resources
    QFile templateFile(":/files/passwords.desktop");
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QString content = QString::fromUtf8(templateFile.readAll());
    templateFile.close();

    // replace placeholders
    QString iconPath = QDir::homePath() + "/.local/share/icons/password.png";
    QFile iconRes(":/password.png");
    if (iconRes.open(QIODevice::ReadOnly)) {
        QDir().mkpath(QDir::homePath() + "/.local/share/icons");
        QFile iconOut(iconPath);
        if (iconOut.open(QIODevice::WriteOnly)) {
            iconOut.write(iconRes.readAll());
            iconOut.close();
        }
        iconRes.close();
    }

    content.replace("%1", exePath);
    content.replace("%2", iconPath);

    QFile outFile(desktopFilePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&outFile);
    out << content;
    outFile.close();

    qDebug() << "Creating a .desktop file at" << desktopFilePath;
    return true;
}

bool Settings::verifyDeleteAllowed(QSqlDatabase &db, QWidget *parent)
{
    QSqlQuery q(db);
    if (!q.exec("SELECT value FROM app_info WHERE key LIKE 'destructive_operation_password_%'")) {
        qWarning() << "Failed to query app_info:" << q.lastError().text();
        return false; // fail safe
    }

    QList<QString> hashes;
    while (q.next()) {
        hashes << q.value(0).toString();
    }

    // Case 1: no destructive passwords set → allow delete
    if (hashes.isEmpty()) {
        return true;
    }

    // Case 2: prompt user for password
    QString enteredPassword = QInputDialog::getText(parent,
                                                    QObject::tr("Confirm Delete"),
                                                    QObject::tr("This database has been setup to require additional authentication\nfor destructive actions.\n\nPlease enter a valid password:"),
                                                    QLineEdit::Password);

    if (enteredPassword.isEmpty()) {
        return false; // user cancelled
    }

    QByteArray enteredBytes = enteredPassword.toUtf8();
    QByteArray enteredHash = QCryptographicHash::hash(enteredBytes, QCryptographicHash::Sha256);
    QString enteredHex = QString(enteredHash.toHex());

    for (const QString &stored : std::as_const(hashes)) {
        if (enteredHex == stored) {
            return true; // match found
        }
    }

    return false; // no match
}
