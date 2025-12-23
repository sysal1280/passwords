#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QSqlDatabase>
#include <QLineEdit>

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
