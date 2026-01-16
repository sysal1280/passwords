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


#include "dbutils.h"
#include "settings.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QStringList>
#include <QSqlError>
#include <QSqlQuery>


Settings::Settings()
{
    QString globalDir;

#ifdef Q_OS_WIN
    // Windows: C:/ProgramData/Passwords
    globalDir = QDir::fromNativeSeparators(qEnvironmentVariable("PROGRAMDATA") + "/Passwords");
#elif defined(Q_OS_MAC)
    // macOS: /Library/Preferences/Passwords
    globalDir = "/Library/Preferences/Passwords";
#else
    // Linux/Unix: /etc/passwords
    globalDir = "/etc/passwords";
#endif

    // Ensure global directory exists (harmless if read‑only)
    QDir(globalDir).mkpath(".");
    QString globalFile = globalDir + "/" + CONFIG_FILENAME;

    // Local per‑user config
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
                qWarning().noquote() << Q_FUNC_INFO << "Could not copy template config to" << configFile;
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
            qWarning().noquote() << Q_FUNC_INFO << "Could not copy wordlist resource to" << wlFile;
        } else {
            QFile::setPermissions(wlFile,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
    }

    settings = std::make_unique<QSettings>(configFile, QSettings::IniFormat);
}

QString Settings::configFilePath() const
{
    if (!settings)
        return QString();

    return settings->fileName();
}

QString Settings::getDefaultDbPath(QWidget* parent) const
{
    // 1. Last used file
    if (QFile::exists(getLastUsedFile())) {
        return getLastUsedFile();
    }

// 2. Global system path (OS-specific)
#ifdef Q_OS_WIN
    // Windows: C:/ProgramData/Passwords/Data
    QString globalPath = QDir::fromNativeSeparators(
        qEnvironmentVariable("PROGRAMDATA") + "/Passwords/Data/passwords"
        );
#elif defined(Q_OS_MAC)
    // macOS: /Library/Application Support/Passwords/passwords
    QString globalPath = "/Library/Application Support/Passwords/passwords";

#else
    // Linux: /var/lib/password/passwords
    QString globalPath = "/var/lib/passwords/passwords";

#endif

    // 3. Local
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(configDir).mkpath(".");
    QString localPath = QDir(configDir).filePath("passwords");

    qInfo().noquote() << QString("Looking for database at %1.").arg(globalPath);
    if (QFile::exists(globalPath)) {
        return globalPath;
    }

    qInfo().noquote() << QString("Looking for database at %1.").arg(localPath);
    if (QFile::exists(localPath)) {
        return localPath;
    }

    // 4. Portable mode: next to the executable
    QString portablePath = QDir(QCoreApplication::applicationDirPath()).filePath("passwords");
    qInfo() << "Looking for database at" << portablePath;
    if (QFile::exists(portablePath)) {
        return portablePath;
    }

    return "";
}

QString Settings::getWordListFile() const
{
    if (!settings)
        return QString();

    settings->beginGroup("Passwords");
    QString file = settings->value("WordList", "").toString().trimmed();
    settings->endGroup();

    // Build full resource path
    QString path = QString(":/wordlist/%1").arg(file);

    // Optional sanity check: ensure a filename was provided
    if (file.isEmpty())
        return QString();
    return path;
}

QString Settings::getPathSeparator() const
{
    if (!settings)
        return " / ";

    settings->beginGroup("Passwords");
    QString val = settings->value("PathSeparator", "\\").toString();
    settings->endGroup();

    // Optional sanity check
    if (val.isEmpty())
        val = " / ";

    return val;
}

void Settings::setLastUsedFile(const QString &filePath)
{
    if (!settings)
        return;

    settings->beginGroup("Main");
    settings->setValue("LastDatabase", filePath);
    settings->endGroup();
}

QString Settings::getLastUsedFile() const
{
    if (!settings)
        return QString();

    settings->beginGroup("Main");
    QString val = settings->value("LastDatabase", "").toString().trimmed();
    settings->endGroup();

    return val;
}

int Settings::getGeneratedPasswordLength() const
{
    if (!settings)
        return 2;

    settings->beginGroup("Passwords");
    int val = settings->value("GeneratedPasswordLength", 2).toInt();
    settings->endGroup();

    // Sanity check: enforce a reasonable range
    if (val < 2 || val > 15)
        val = 2;

    return val;
}

void Settings::setGeneratedPasswordLength(int i)
{
    if (!settings)
        return;

    settings->beginGroup("Passwords");
    settings->setValue("GeneratedPasswordLength", i);
    settings->endGroup();
}

void Settings::setRandomNoiseLength(int length)
{
    if (!settings)
        return;

    settings->beginGroup("Main");
    settings->setValue("LastRandomNoiseLength", length);
    settings->endGroup();
}

int Settings::getRandomNoiseLength() const
{
    if (!settings)
        return 0;

    settings->beginGroup("Main");
    return settings->value("LastRandomNoiseLength", 32).toInt();
}

void Settings::setRandomNoiseOption(int index)
{
    if (!settings)
        return;

    settings->beginGroup("Main");
    settings->setValue("LastRandomNoise", index);
    settings->endGroup();
}

int Settings::getRandomNoiseOption() const
{
    if (!settings)
        return 0;

    settings->beginGroup("Main");
    return settings->value("LastRandomNoise", 0).toInt();
}

bool Settings::getBackupDatabase() const
{
    if (!settings)
        return true;

    settings->beginGroup("Main");
    QString val = settings->value("BackupDatabase", "yes")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getKillGpgAgent(const QString &key) const
{
    if (!settings)
        return false;

    settings->beginGroup("Passwords");
    QString val = settings->value(key, "yes")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getLoginPreference() const
{
    if (!settings)
        return false; // safe fallback

    settings->beginGroup("Main");
    QString val = settings->value("RequireChallenge", false)
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getAskClose() const
{
    if (!settings)
        return true; // safe default

    settings->beginGroup("Main");
    QString val = settings->value("AskBeforeClosing", "yes")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getDragDropPrompt() const
{
    if (!settings)
        return true;

    settings->beginGroup("Passwords");
    QString val = settings->value("DragDropPrompt", "yes")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getCloseHelpServer() const
{
    if (!settings)
        return true;

    settings->beginGroup("Help");
    QString val = settings->value("CloseServer", "yes")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::getDebugMode() const
{
    if (!settings)
        return true;

    settings->beginGroup("Main");
    QString val = settings->value("Debug", "no")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::closeOnCopy() const
{
    if (!settings)
        return true;

    settings->beginGroup("Passwords");
    QString val = settings->value("CloseOnCopy", "no")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

bool Settings::openCategoryDblClick() const
{
    if (!settings)
        return true; // safe default

    settings->beginGroup("Categories");
    QString val = settings->value("DoubleClickOpen", "false")
                      .toString()
                      .trimmed()
                      .toLower();
    settings->endGroup();

    static const QSet<QString> truthy = {"1", "true", "yes", "on"};
    return truthy.contains(val);
}

int Settings::getAutoCloseSeconds() const
{
    if (!settings)
        return 0;

    settings->beginGroup("Passwords");
    int val = settings->value("AutoClose", 0).toInt();
    settings->endGroup();

    return val;
}

int Settings::getMaxRecentResults() const
{
    if (!settings)
        return 15;

    settings->beginGroup("Search");
    int val = settings->value("MaxRecentResults", 15).toInt();
    settings->endGroup();

    if (val < 0)
        val = 15;

    return val;
}

int Settings::getMaxPopularResults() const
{
    if (!settings)
        return 0;

    settings->beginGroup("Search");
    int val = settings->value("MaxPopularResults", 15).toInt();
    settings->endGroup();

    // sanity check
    if (val < 0)
        val = 15;

    return val;
}

int Settings::getMaxBackups() const
{
    if (!settings)
        return 2;

    settings->beginGroup("Main");
    int val = settings->value("MaxBackups", 0).toInt();
    settings->endGroup();

    return val;
}

int Settings::getHelpPort() const
{
    if (!settings)
        return 1280;

    settings->beginGroup("Help");
    int val = settings->value("Port", 1280).toInt();
    settings->endGroup();

    // Accept only non‑privileged ports: 1024–65535
    if (val < 1024 || val > 65535)
        val = 1280;

    return val;
}

QLineEdit::EchoMode Settings::getEchoMode() const
{
    if (!settings)
        return QLineEdit::Password;

    settings->beginGroup("Main");
    int val = settings->value("EchoMode", QLineEdit::Password).toInt();
    settings->endGroup();

    // Sanity check: ensure val is a valid QLineEdit::EchoMode
    if (val < QLineEdit::Normal || val > QLineEdit::PasswordEchoOnEdit)
        val = QLineEdit::Password;

    return static_cast<QLineEdit::EchoMode>(val);
}

QString Settings::getShellCommand() const
{
    if (!settings)
        return QString();

    settings->beginGroup("Main");
    QString cmd = settings->value("ShellCommand").toString();
    settings->endGroup();
    return cmd;
}

QString Settings::getShellArguments() const
{
    if (!settings)
        return QString();

    settings->beginGroup("Main");
    QString args = settings->value("ShellArguments").toString(); // or "ShellCommand" for your test
    settings->endGroup();
    return args;
}

void Settings::saveMainWindowState(QMainWindow *window)
{
    if (!window)
        return;

    QSettings localSettings;

    localSettings.beginGroup("MainWindow");
    localSettings.setValue("geometry", window->saveGeometry());
    localSettings.setValue("state", window->saveState());
    localSettings.endGroup();
}

void Settings::restoreMainWindowState(QMainWindow *window)
{
    if (!window)
        return;

    QSettings localSettings;

    localSettings.beginGroup("MainWindow");

    if (localSettings.contains("geometry"))
        window->restoreGeometry(localSettings.value("geometry").toByteArray());

    if (localSettings.contains("state"))
        window->restoreState(localSettings.value("state").toByteArray());

    localSettings.endGroup();
}

void Settings::saveSplitterState(QSplitter *splitter, const QString &name)
{
    if (!splitter)
        return;

    QSettings localSettings;

    localSettings.beginGroup("Splitters");
    localSettings.setValue(name, splitter->saveState());
    localSettings.endGroup();
}

void Settings::restoreSplitterState(QSplitter *splitter, const QString &name)
{
    if (!splitter)
        return;

    QSettings localSettings;

    localSettings.beginGroup("Splitters");

    if (localSettings.contains(name)) {
        splitter->restoreState(localSettings.value(name).toByteArray());
    }

    localSettings.endGroup();
}

bool Settings::verifyDeleteAllowed(QSqlDatabase &db, QWidget *parent)
{
    // Destructive password prompt

    QSqlQuery q(db);
    if (!q.exec("SELECT value FROM app_info WHERE key LIKE 'destructive_operation_password_%'")) {
        showQueryError(parent, q, Q_FUNC_INFO);
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
        return false;
    }

    QByteArray enteredBytes = enteredPassword.toUtf8();
    QByteArray enteredHash = QCryptographicHash::hash(enteredBytes, QCryptographicHash::Sha256);
    QString enteredHex = QString(enteredHash.toHex());

    for (const QString &stored : std::as_const(hashes)) {
        if (enteredHex == stored) {
            return true;
        }
    }

    return false;
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

    qInfo().noquote() << Q_FUNC_INFO << "Creating a .desktop file at" << desktopFilePath;
    return true;
}

void Settings::deleteSection(const QString &section)
{
    if (!settings)
        return;

    settings->beginGroup(section);
    bool exists = !settings->allKeys().isEmpty();
    settings->endGroup();

    if (exists) {
        settings->beginGroup(section);
        settings->remove("");
        settings->endGroup();

        settings->remove(section);
    }
}
