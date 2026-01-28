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


class Settings
{
public:
    Settings();

    // Core configuration paths
    QString configFilePath() const;
    QString getDefaultDbPath(QWidget *parent) const;
    QString getWordListFile() const;
    QString getPathSeparator() const;

    // Last used file
    void setLastUsedFile(const QString &filePath);
    QString getLastUsedFile() const;

    // Generated password settings
    int getGeneratedPasswordLength() const;
    void setGeneratedPasswordLength(int i);

    // Random noise settings
    void setRandomNoiseLength(int length);
    int getRandomNoiseLength() const;
    void setRandomNoiseOption(int index);
    int getRandomNoiseOption() const;

    // Boolean preferences
    bool getBackupDatabase() const;
    bool getKillGpgAgent(const QString &key = "KillGPGAgent") const;
    bool getLoginPreference() const;
    bool getAskClose() const;
    bool getDragDropPrompt() const;
    bool getCloseHelpServer() const;
    bool getDebugMode() const;
    bool closeOnCopy() const;
    bool openCategoryDblClick() const;

    // Numeric preferences
    int getAutoCloseSeconds() const;
    int getMaxRecentResults() const;
    int getMaxPopularResults() const;
    int getMaxBackups() const;
    int getHelpPort() const;

    // Echo mode
    QLineEdit::EchoMode getEchoMode() const;

    // Shell command configuration
    QString getShellCommand() const;
    QString getShellArguments() const;

    // UI state management
    void saveMainWindowState(QMainWindow *window);
    void restoreMainWindowState(QMainWindow *window);
    void saveSplitterState(QSplitter *splitter, const QString &name);
    void restoreSplitterState(QSplitter *splitter, const QString &name);

    // Database-related
    bool verifyDeleteAllowed(QSqlDatabase &db, QWidget *parent);

    // Miscellaneous
    bool createUserDesktopFile();
    void deleteSection(const QString &section);

private:
    QString configDir;
    QString configFile;
    std::unique_ptr<QSettings> settings;

    static constexpr const char* CONFIG_FILENAME = "passwords.conf";
    static constexpr const char* WORDLIST_FILENAME = "wordlist.rcc";
};

#endif // SETTINGS_H
