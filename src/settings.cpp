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
    // --- Build candidate paths ---
#ifdef Q_OS_WIN
    // Windows: ProgramData or GenericConfigLocation
    QString globalFile = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                             .filePath(CONFIG_FILENAME);

#elif defined(Q_OS_MAC)
    // macOS: /Library/Preferences is the standard global prefs location
    QString globalFile = QStringLiteral("/Library/Preferences/passwords/") + CONFIG_FILENAME;

#else
    // Linux/Unix: /etc/yourapp
    QString globalFile = QStringLiteral("/etc/passwords/") + CONFIG_FILENAME;
#endif


    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(configDir).mkpath(".");
    QString localFile = configDir + "/" + CONFIG_FILENAME;

    // --- Choose global if it exists, otherwise local ---
    if (QFile::exists(globalFile)) {
        configFile = globalFile;
    } else {
        configFile = localFile;
        // ensure local template exists
        if (!QFile::exists(configFile)) {
            if (!QFile::copy(":/files/passwords.conf", configFile)) {
                qWarning() << "Could not copy template config to" << configFile;
            } else {
                QFile::setPermissions(configFile,
                                      QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            }
        }
    }

    // --- Wordlist resource: always beside configFile ---
    QFileInfo cfgInfo(configFile);
    QString wlFile = cfgInfo.dir().filePath(WORDLIST_FILENAME);

    if (!QFile::exists(wlFile)) {
        if (!QFile::copy(":/files/wordlist.rcc", wlFile)) {
            qWarning() << "Could not copy wordlist resource to" << wlFile;
        } else {
            QFile::setPermissions(wlFile,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
    }

    settings = std::make_unique<QSettings>(configFile, QSettings::IniFormat);
}


QString Settings::configFilePath()
{
    return settings->fileName();
}

void Settings::setLastUsedFile(const QString &filePath)
{
    settings->beginGroup("General");
    settings->setValue("LastDatabase", filePath);
    settings->endGroup();
    settings->sync();
}

QString Settings::getLastUsedFile()
{
    settings->beginGroup("General");
    QString val = settings->value("LastDatabase", "").toString();
    settings->endGroup();

    return val;
}

QString Settings::getWordListFile()
{
    settings->beginGroup("Passwords");
    QString val = QString(":/wordlist/%1").arg(settings->value("WordList", "").toString());
    settings->endGroup();

    return val;
}

QString Settings::getPathSeparator() const
{
    settings->beginGroup("Passwords");
    QString val = settings->value("PathSeparator", " / ").toString();
    settings->endGroup();
    return val;
}


bool Settings::getBackupDatabase()
{
    settings->beginGroup("General");
    QString val = settings->value("BackupDatabase", "yes").toString().trimmed().toLower();
    settings->endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getCloseHelpServer()
{
    settings->beginGroup("Help");
    QString val = settings->value("CloseServer", "yes").toString().trimmed().toLower();
    settings->endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getDragDropPrompt()
{
    settings->beginGroup("Passwords");
    QString val = settings->value("DragDropPrompt", "yes").toString().trimmed().toLower();
    settings->endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getKillGpgAgent()
{
    settings->beginGroup("Passwords");
    QString val = settings->value("KillGPGAgent", "no").toString().trimmed().toLower();
    settings->endGroup();

    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

int Settings::getGeneratedPasswordLength()
{
    settings->beginGroup("Passwords");
    int val = settings->value("GeneratedPasswordLength", 2).toInt();
    settings->endGroup();
    return val;
}

QLineEdit::EchoMode Settings::getEchoMode()
{
    settings->beginGroup("General");
    int val = settings->value("EchoMode", QLineEdit::Password).toInt();
    settings->endGroup();
    return static_cast<QLineEdit::EchoMode>(val);
}

int Settings::getHelpPort()
{
    settings->beginGroup("Help");
    int val = settings->value("Port", 1280).toInt();
    settings->endGroup();
    return val;
}

int Settings::getMaxRecentResults()
{
    settings->beginGroup("Search");
    int val = settings->value("MaxRecentResults", 15).toInt();
    settings->endGroup();
    return val;
}

int Settings::getMaxPopularResults()
{
    settings->beginGroup("Search");
    int val = settings->value("MaxPopularResults", 15).toInt();
    settings->endGroup();
    return val;
}

int Settings::getAutoCloseSeconds()
{
    settings->beginGroup("Passwords");
    int val = settings->value("AutoClose", 0).toInt();
    settings->endGroup();
    return val;
}

bool Settings::getLoginPreference()
{
    settings->beginGroup("General");
    QString val = settings->value("RequireChallenge", "yes").toString().trimmed().toLower();
    settings->endGroup();
    // Accept common truthy values
    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getAskClose()
{
    settings->beginGroup("General");
    QString val = settings->value("AskBeforeClosing", "yes").toString().trimmed().toLower();
    settings->endGroup();
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

    settings->beginGroup("Passwords");
    settings->setValue("GeneratedPasswordLength", i);
    settings->endGroup();
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
    settings->beginGroup("MainWindow");
    settings->setValue("geometry", window->saveGeometry());
    settings->setValue("state", window->saveState());
    settings->endGroup();
}

// Restore MainWindow geometry/state
void Settings::restoreMainWindowState(QMainWindow *window)
{
    settings->beginGroup("MainWindow");
    window->restoreGeometry(settings->value("geometry").toByteArray());
    window->restoreState(settings->value("state").toByteArray());
    settings->endGroup();
}

// Save splitter state
void Settings::saveSplitterState(QSplitter *splitter, const QString &name)
{
    if (!splitter)
    {
        return;
    }

    QByteArray state = splitter->saveState();
    qDebug() << "Saving splitter" << name << "state size:" << state.size();

    settings->beginGroup("Splitters");
    settings->setValue(name, state);
    settings->endGroup();
    settings->sync();
}

// Restore splitter state
void Settings::restoreSplitterState(QSplitter *splitter, const QString &name)
{
    settings->beginGroup("Splitters");
    splitter->restoreState(settings->value(name).toByteArray());
    settings->endGroup();
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
