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
#include "settings.h"
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

    // JSON population
    void populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui);

    void createCategory(const QString& categoryName = QString());
    void importApplicationsFromFile(const QString &filePath);
    void renameCategory();
    void showAboutDlg();

    void openCategory(QTreeWidgetItem *item, int column);
    void setBookmark(bool checked);

    QString formatOtp(const QString& otp);

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

    void showPasswordsContextMenu(const QPoint &pos);

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
