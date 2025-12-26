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


#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDir>
#include <QLineEdit>
#include <QMainWindow>
#include <QSettings>
#include <QSplitter>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QString>
#include <QWidget>

/**
 * @brief Settings class wraps QSettings to store/retrieve application preferences.
 *
 * The settings are stored in an INI file named "passwords.conf"
 * inside the writable AppConfigLocation directory.
 */
class Settings
{
public:
    Settings();

    // Example getters/setters
     QString configFilePath();

    void setLastUsedFile(const QString &filePath);
     QString getLastUsedFile();
     int getGeneratedPasswordLength();
     void setGeneratedPasswordLength(int i);
     QString getDefaultDbPath(QWidget* parent);
     bool getBackupDatabase();
     bool getKillGpgAgent(const QString &key = "KillGPGAgent");
     bool getLoginPreference();
     bool getAskClose();
     void saveMainWindowState(QMainWindow *window);
     void restoreMainWindowState(QMainWindow *window);
     int getAutoCloseSeconds();
     int getMaxRecentResults();
     int getMaxPopularResults();
     QString getPathSeparator() const;
     bool getDragDropPrompt();
     QString getWordListFile();
     bool getCloseHelpServer();
     bool getDebugMode();
     void saveSplitterState(QSplitter *splitter, const QString &name);
     void restoreSplitterState(QSplitter *splitter, const QString &name);
     bool createUserDesktopFile();
     bool verifyDeleteAllowed(QSqlDatabase &db, QWidget *parent);
     int getHelpPort();
     QLineEdit::EchoMode getEchoMode();

private:
    QString configDir;
    QString configFile;
    std::unique_ptr<QSettings> settings;

     static constexpr const char* CONFIG_FILENAME = "passwords.conf";
     static constexpr const char* WORDLIST_FILENAME = "wordlist.rcc";
};

#endif // SETTINGS_H
