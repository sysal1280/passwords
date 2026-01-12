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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "keyentry.h"
#include "settings.h"
#include <QDateTime>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QProgressBar>
#include <QSplitter>
#include <QString>
#include <QSqlDatabase>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace Ui {
class MainWindow;
}

// Simple struct for search results
struct SearchResult {
    int id;
    int categoryId;
    QString appName;
    QString description;
    QString categoryName;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Application signature and schema version
    static constexpr const char* APP_SIGNATURE   = "Asteraceae";
    static constexpr const char* SCHEMA_VERSION = "1";

    // Utility functions
    static QByteArray base32Decode(const QString &base32);
    QList<KeyEntry> fetchKeys() const;

    QString userName;
    QByteArray appKey = "";
    int openedCredentialID = -1;

    bool showDebugMessages = false;
    bool abortingStartup = false;


    QLabel *countdownLabel = nullptr;
    QProgressBar *countdownProgress = nullptr;

    // Public methods
    bool initDb();
    bool openDatabase(const QString &fileName = QString());
    void launchHelperProcess(const QString &page);
    int countExportedWithoutEdits();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:

    // Other slots
    void loadCategories();
    void clearScrollArea();
    void updateFields();
    void updateCountdown();
    void populateBookmarksMenu();
    void search(const QString &text);
    void search(int appId);
    void searchPopular();
    void searchRecent();
    void init();
    bool killGpgAgent();
    void setupAlignedTimer();

private:
    Ui::MainWindow *ui = nullptr;

    bool firstShow = true;

    // Crypto helpers
    QByteArray intToBytes(quint64 counter);
    QByteArray hmacSha1(const QByteArray &key, const QByteArray &message);
    QString generateTOTP(const QByteArray &secret, int digits = 6, int step = 30);

    // UI helpers
    //void setActionIcon(QAction *action, const QString &iconPath);
    QTreeWidgetItem* findCategoryItemRecursive(QTreeWidgetItem *item, int categoryId);
    QString buildItemPath(QTreeWidgetItem *item) const;
    QString getItemPath(QTreeWidgetItem *item, int column = 0);
    QString buildCategoryPath(int categoryId, const QString &appKey, QSqlDatabase &db);
    void moveCategory(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem, bool skipPrompt);
    void moveCategories(const QList<QTreeWidgetItem*> &items, QTreeWidgetItem *targetItem);
    void selectInTreeWidgets(int categoryId, int appId);

    // Tree persistence
    void saveTreeToDb(QTreeWidget* tree, QSqlDatabase& db);
    void saveItemToDb(QTreeWidgetItem* item, QSqlDatabase& db, const QVariant& parentId);

    // Item creation
    QTreeWidgetItem* makeItemFromApplication(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromNote(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromFile(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromCredit(QSqlQuery& query);

    // JSON population
    void populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui);
    QString shortenUrlForDisplay(const QString& url, int maxLen = 55);

    void createCategory(const QString& categoryName = QString());
    void importApplicationsFromFile(const QString &filePath);
    void renameCategory();
    //QString appKeyFilePath() const;

    QByteArray loadOrCreateAppKey();
    void checkGpgKeys();
    void initDbMetadata();

    void openCategory(QTreeWidgetItem *item, int column);
    void openCategoryFromCurrent(QTreeWidgetItem* current, QTreeWidgetItem*);

    void setBookmark(bool checked);

    QString formatOtp(const QString& otp);
    void moveCategoryWrapper(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem);


    void wipeFile(const QString &path, int passes = 2);

    void newPassword();
    void openPassword(QTreeWidgetItem *item);
    void editPassword(QTreeWidgetItem *item);
    void exportPassword(QTreeWidgetItem *item);
    void showAuditLog(QTreeWidgetItem *item);
    void deleteCategory(QTreeWidgetItem *item);
    void addSearchTerms(QTreeWidgetItem *item);
    void keyList();
    void deletePassword(QTreeWidgetItem *item);
    void encryptMessage();
    void decryptMessage();
    void encryptFile();
    void decryptFile();
    void exportedWithoutEdits();
    void insertAuditRow(int applicationId, const QString &user, const QString &host, const QString &action);
    void NotChangedSince(const QDateTime &cutoff);
    void showPasswordsContextMenu(const QPoint &pos);
    bool tryOpenPasswordPath(const QString &path);

    void decryptWithGpg(
        const QByteArray &encrypted,
        std::function<void(const QByteArray&)> onSuccess,
        std::function<void(const QString&)> onMissingKey,
        std::function<void(const QString&)> onFailure
        );

    // Members
    static const QMap<QString, QString> headerMap;
    QSplitter *vSplitter = nullptr;
    QSplitter *hSplitter = nullptr;
    QTimer *autoCloseTimer = nullptr;
    QTimer *countdownTimer = nullptr;
    QTimer *alignedTimer = nullptr;
    QIcon closedIcon;
    QIcon openIcon;
    QProcess *helperProcess = nullptr;

    Settings settings;
};

#endif // MAINWINDOW_H
