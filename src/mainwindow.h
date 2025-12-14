#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QString>
#include <QLabel>
#include <QProgressBar>
#include <QMap>
#include <QSplitter>
#include <QProcess>
#include <QIcon>

#include "keyentry.h"

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

    // Public state
    QString userName;
    QByteArray appKey = "";
    int openedCredentialID = -1;
    bool showDebugMessages = false;

    QLabel *countdownLabel = nullptr;
    QProgressBar *countdownProgress = nullptr;

    // Public methods
    void initDb();
    bool openDatabase(const QString &fileName = QString());
    void launchHelperProcess(const QString &page);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // Actions
    void on_actionAbout_Qt_triggered();
    void on_actionNew_Password_triggered();
    void on_actionNew_Category_triggered();
    void on_actionShow_Password_toggled(bool arg1);
    void on_actionShow_debug_messages_triggered(bool checked);
    void on_actionKey_List_triggered();
    void on_actionAudit_Log_triggered();
    void on_actionRefresh_Categories_triggered();
    void on_actionGenerate_Password_triggered();
    void on_actionEncrypt_message_triggered();
    void on_actionDecrypt_message_triggered();
    void on_actionEncrypt_File_triggered();
    void on_actionDecrypt_File_triggered();
    void on_actionOpen_Database_triggered();

    void on_actionDelete_Password_triggered();
    void on_actionDelete_Category_triggered();
    void on_actionEdit_Password_triggered();
    void on_actionExport_Password_triggered();
    void on_actionAdd_Search_triggered();

    // Tree widget handlers

    void on_treeWidget_2_itemActivated(QTreeWidgetItem *item, int column);
    void on_treeWidget_2_customContextMenuRequested(const QPoint &pos);

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

    // Crypto helpers
    QByteArray intToBytes(quint64 counter);
    QByteArray hmacSha1(const QByteArray &key, const QByteArray &message);
    QString generateTOTP(const QByteArray &secret, int digits = 6, int step = 30);

    // UI helpers
    void setActionIcon(QAction *action, const QString &iconPath);
    QTreeWidgetItem* findCategoryItemRecursive(QTreeWidgetItem *item, int categoryId);
    QString buildItemPath(QTreeWidgetItem *item) const;
    QString getItemPath(QTreeWidgetItem *item, int column = 0);
    QString buildCategoryPath(int categoryId, const QString &appKey, QSqlDatabase &db);
    void moveCategory(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem);
    void selectInTreeWidgets(int categoryId, int appId);

    // Tree persistence
    void saveTreeToDb(QTreeWidget* tree, QSqlDatabase& db);
    void saveItemToDb(QTreeWidgetItem* item, QSqlDatabase& db, const QVariant& parentId);

    // Item creation
    QTreeWidgetItem* makeItemFromApplication(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromNote(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromFile(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromCredit(QSqlQuery& query);

    // JSON parsing
    void parseJson(const QByteArray &jsonData);
    void parseJsonNote(const QByteArray &jsonData);
    void parseJsonFile(const QByteArray &jsonData, QString filename);

    // JSON population
    void populateFromJson(const QByteArray &jsonData, Ui::MainWindow *ui);
    void populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui);

    void createCategory(const QString& categoryName = QString());
    void importApplicationsFromFile(const QString &filePath);
    void renameCategory();
    void showAboutDlg();

    void openCategory(QTreeWidgetItem *item, int column);
    void setBookmark(bool checked);

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
};

#endif // MAINWINDOW_H
