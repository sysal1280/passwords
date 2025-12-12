#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qsqldatabase.h>
#include <qtreewidget.h>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QString>
#include <QLabel>
#include <QProgressBar>
#include <QMap>
#include "keyentry.h"
#include <QSplitter>
#include "settings.h"

namespace Ui {
class MainWindow;
}

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
    QString userName;
    QString mode = "application";
    QByteArray appKey = "";

    int openedCredentialID;
    QLabel *countdownLabel;

    QProgressBar *countdownProgress;
        bool showDebugMessages = false;


    static QByteArray base32Decode(const QString &base32);
    void initDb();
    void openDatabase(const QString &fileName = QString());

    QList<KeyEntry> fetchKeys() const;

protected:
    void closeEvent(QCloseEvent *event) override;  // declare override

private slots:
    void on_actionAbout_triggered();  // ← Add this line

    void on_actionNew_Password_triggered();

    void loadCategories();

    void saveTreeToDb(QTreeWidget* tree, QSqlDatabase& db);

    void saveItemToDb(QTreeWidgetItem* item, QSqlDatabase& db, const QVariant& parentId);

    void on_actionNew_Category_triggered();

    void on_treeWidget_itemActivated(QTreeWidgetItem *item, int column);

    void on_treeWidget_2_itemActivated(QTreeWidgetItem *item, int column);

    void clearScrollArea();

    void parseJson(const QByteArray &jsonData);

    void populateFromJson(const QByteArray &jsonData, Ui::MainWindow *ui);

    bool killGpgAgent();

    void setupAlignedTimer();

    void updateFields();

    void on_actionShow_Password_toggled(bool arg1);

    void on_treeWidget_2_customContextMenuRequested(const QPoint &pos);

    void updateCountdown();

    void on_actionShow_debug_messages_triggered(bool checked);

    //QString getDefaultDbPath(QWidget* parent = nullptr);

    void changeModes(QString mode);

    void on_actionKey_List_triggered();

    void on_actionClose_triggered();

    void on_actionAudit_Log_triggered();

    void on_actionRefresh_Categories_triggered();

    void on_actionGenerate_Password_triggered();

    void on_actionEncrypt_message_triggered();

    void on_actionDecrypt_message_triggered();

    void on_actionEncrypt_File_triggered();

    void on_actionDecrypt_File_triggered();

    void on_actionOpen_Database_triggered();

    void init();

    void search(const QString &text);

    void search(int appId);

    QTreeWidgetItem* findCategoryItemRecursive(QTreeWidgetItem *item, int categoryId);

    void populateBookmarksMenu();

    QString buildItemPath(QTreeWidgetItem *item) const;

    void on_actionAbout_Qt_triggered();

    void on_actionBookmark_triggered(bool checked);

    void on_actionDelete_Password_triggered();

    void on_actionDelete_Category_triggered();

    void on_treeWidget_customContextMenuRequested(const QPoint &pos);

    void searchPopular();

    void on_actionPopular_triggered();

    void searchRecent();

    void on_actionRecent_triggered();

    void on_actionEdit_Password_triggered();

    void on_actionPreferences_triggered();

    void on_actionSystem_Information_triggered();

    void on_actionExport_Password_triggered();

    void moveCategory(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem);

    void importApplicationsFromFile(const QString &filePath);

    void on_actionImport_triggered();

    void on_actionRename_triggered();

    void on_actionAdd_Search_triggered();
    void keyPressEvent(QKeyEvent *event) override;

    void on_actionOnline_Documentation_triggered();

    void on_actionDonate_triggered();

private:
    Ui::MainWindow *ui;
    QByteArray intToBytes(quint64 counter);
    QByteArray hmacSha1(const QByteArray &key, const QByteArray &message);
    QString generateTOTP(const QByteArray &secret, int digits = 6, int step = 30);
    static const QMap<QString, QString> headerMap;
    QSplitter *vSplitter;   // vertical splitter
    QSplitter *hSplitter;   // horizontal splitter
    QTimer *autoCloseTimer = nullptr;
    QTimer *countdownTimer = nullptr;
    QTimer *alignedTimer = nullptr;
    QIcon closedIcon;
    QIcon openIcon;

    void setActionIcon(QAction *action, const QString &iconPath);

    QTreeWidgetItem* makeItemFromApplication(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromNote(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromFile(QSqlQuery& query);
    QTreeWidgetItem* makeItemFromCredit(QSqlQuery& query);

    QString getItemPath(QTreeWidgetItem *item, int column = 0);
    QString buildCategoryPath(int categoryId, const QString &appKey, QSqlDatabase &db);

    // Mode-specific JSON parsing
    void parseJsonApplication(const QByteArray &jsonData);
    void parseJsonNote(const QByteArray &jsonData);
    void parseJsonFile(const QByteArray &jsonData, QString filename);
    void parseJsonCredit(const QByteArray &jsonData);
    void createCategory(const QString& categoryName = QString());
    void launchHelperProcess(const QString &page);

    // Mode-specific UI population
    void populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui);
    void populateFromJsonNote(const QByteArray &jsonData, Ui::MainWindow *ui);
    void populateFromJsonFile(const QByteArray &jsonData, Ui::MainWindow *ui);
    void populateFromJsonCredit(const QByteArray &jsonData, Ui::MainWindow *ui);


         void selectInTreeWidgets(int categoryId, int appId);


};

#endif // MAINWINDOW_H
