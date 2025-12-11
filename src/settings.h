#ifndef SETTINGS_H
#define SETTINGS_H
#pragma once
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QSqlDatabase>

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
    static QString configFilePath();

    static void setLastUsedFile(const QString &filePath);
    static QString getLastUsedFile();
    static int getGeneratedPasswordLength();
    static void setGeneratedPasswordLength(int i);
    static QString getDefaultDbPath(QWidget* parent);
    static bool getBackupDatabase();
    static bool getKillGpgAgent();
    static bool getLoginPreference();
    static bool getAskClose();
    static void saveMainWindowState(QMainWindow *window);
    static void restoreMainWindowState(QMainWindow *window);
    static int getAutoCloseSeconds();
    static int getMaxRecentResults();
    static int getMaxPopularResults();
    static QString getPathSeparator();
    static bool getDragDropPrompt();
    static QString getWordListFile();

    static void saveSplitterState(QSplitter *splitter, const QString &name);
    static void restoreSplitterState(QSplitter *splitter, const QString &name);
    static bool createUserDesktopFile();
    static bool verifyDeleteAllowed(QSqlDatabase &db, QWidget *parent);

private:
    QString configDir;
    QString configFile;
    QSettings *settings;

    static constexpr const char* CONFIG_FILENAME = "passwords.conf";
    static constexpr const char* WORDLIST_FILENAME = "wordlist.rc";
};

#endif // SETTINGS_H
