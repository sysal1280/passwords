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


#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "dataobfuscator.h"
#include "systeminfodialog.h"
#include "aboutdialog.h"
#include "categoryproperties.h"
#include "categorydialog.h"
#include "constants.h"
#include "dbutils.h"
#include "debugutils.h"
#include "droplabel.h"
#include "encryptfiledialog.h"
#include "gitversion.h"
#include "gpgcheck.h"
#include "newpassworddialog.h"
#include "passworddialog.h"
#include "passwordgenerator.h"
#include "plaintextedit.h"
#include "preferencesdialog.h"
#include "randomnoisedialog.h"
#include "settings.h"
#include "utils.h"
#include "watermarkedtreewidget.h"

#include <QActionGroup>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QDateEdit>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMap>
#include <QMessageBox>
#include <QProcess>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResource>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/qsqlquerymodel.h>
#include <QWidgetAction>

const QMap<QString, QString> MainWindow::headerMap = {
    { "application", "Passwords" },
    { "note",        "Notes" },
    { "file",        "Files" },
    { "credit",      "Credit Cards" }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startDebuggerMonitor(this, 7000);

#ifdef Q_OS_WIN
    userName = QString::fromLocal8Bit(qgetenv("USERNAME"));
#else
    userName = QString::fromLocal8Bit(qgetenv("USER"));
#endif

    closedIcon = QIcon(":/menus/glyphs/folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg");
    openIcon   = QIcon(":/menus/glyphs/folder_open_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg");

    // Horizontal splitter for the two tree widgets
    hSplitter = new QSplitter(Qt::Horizontal, this);
    hSplitter->addWidget(ui->treeWidget);
    hSplitter->addWidget(ui->treeWidget_2);

    // Vertical splitter to stack the horizontal splitter above the scroll area
    vSplitter = new QSplitter(Qt::Vertical, this);
    vSplitter->addWidget(hSplitter);
    vSplitter->addWidget(ui->scrollArea);

    // Central widget with line edit above the splitters
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);   // gap around edges
    layout->addWidget(ui->lineEditSearch);
    layout->addWidget(vSplitter);
    setCentralWidget(central);

    // Restore saved window and splitter states
    settings.restoreMainWindowState(this);
    settings.restoreSplitterState(vSplitter, "vsplit");
    settings.restoreSplitterState(hSplitter, "hsplit");

    ui->treeWidget->setHeaderLabels({ tr("Categories") });
    ui->treeWidget->header()->setStretchLastSection(true);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget->setAcceptDrops(true);
    ui->treeWidget->setDragDropMode(QAbstractItemView::DropOnly);
    ui->treeWidget->setDropIndicatorShown(true);
    ui->treeWidget->setAnimated(true);
    ui->treeWidget->setWatermarkText("No Categories");

    ui->treeWidget_2->setHeaderLabels({ tr("Passwords") });
    ui->treeWidget_2->header()->setStretchLastSection(true);
    ui->treeWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget_2->setDragEnabled(true);
    ui->treeWidget_2->setDragDropMode(QAbstractItemView::DragOnly);
    ui->treeWidget_2->setAnimated(true);
    ui->treeWidget_2->setWatermarkText("No Passwords");

    //setup glyphs
    QHash<QAction*, QString> actionIcons = {
        { ui->actionGenerate_Password,     ":/menus/glyphs/password_24dp_1F1F1F.svg" },
        { ui->actionEncrypt_message,       ":/menus/glyphs/encrypted_add_circle_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionDecrypt_message,       ":/menus/glyphs/encrypted_minus_circle_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionKey_List,              ":/menus/glyphs/passkey_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionRecent,                ":/menus/glyphs/history_2_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionPopular,               ":/menus/glyphs/local_fire_department_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionAbout,                 ":/menus/glyphs/lock_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionAbout_Qt,              ":/menus/glyphs/qt.svg" },
        { ui->actionPreferences,           ":/menus/glyphs/settings_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionOpen_Database,         ":/menus/glyphs/database_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionDelete_Password,       ":/menus/glyphs/delete_forever_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionEdit_Password,         ":/menus/glyphs/edit_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionOpen_Password,         ":/menus/glyphs/lock_open_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionAudit_Log,             ":/menus/glyphs/footprint_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionNew_Category,          ":/menus/glyphs/create_new_folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionNew_Password,          ":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionDelete_Category,       ":/menus/glyphs/folder_delete_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionRefresh_Categories,    ":/menus/glyphs/refresh_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionDecrypt_File,          ":/menus/glyphs/encrypted_off_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionEncrypt_File,          ":/menus/glyphs/encrypted_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionSystem_Information,    ":/menus/glyphs/info_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionExport_Password,       ":/menus/glyphs/export_notes_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionImport,                ":/menus/glyphs/file_json_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionRename,                ":/menus/glyphs/bookmark_manager_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionAdd_Search,            ":/menus/glyphs/loupe_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionDonate,                ":/menus/glyphs/favorite_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionExported_Passwords,    ":/menus/glyphs/table_view_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionLast_Edited,           ":/menus/glyphs/table_view_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionRandom_Noise,          ":/menus/glyphs/grain_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionClear_GPG_Passphrase_Cache, ":/menus/glyphs/lock_reset_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionMaintenance,           ":/menus/glyphs/build_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
        { ui->actionOnline_Documentation,  ":/menus/glyphs/help_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" }
    };

    for (auto it = actionIcons.begin(); it != actionIcons.end(); ++it) {
        it.key()->setIcon(QIcon(it.value()));
    }

    /*
     * Close button.
     * Handled differently so it can be styled separate.
     */
    QLabel *closeLabel = new QLabel("Close");
    closeLabel->setObjectName("actionCloseLabel");
    QWidgetAction *closeAction = new QWidgetAction(this);
    closeAction->setMenuRole(QAction::QuitRole);
    closeAction->setDefaultWidget(closeLabel);
    closeAction->setStatusTip(tr("Exit the application"));
    ui->menuDatabase->addAction(closeAction);

    //properties
    ui->actionAbout->setMenuRole(QAction::AboutQtRole);
    ui->actionAbout_Qt->setMenuRole(QAction::AboutQtRole);
    ui->actionPreferences->setMenuRole(QAction::PreferencesRole);

    ui->actionOpen_Database->setStatusTip(tr("Open an existing database"));
    ui->actionPreferences->setStatusTip(tr("Change application settings"));
    ui->actionMaintenance->setStatusTip(tr("Verify database integrity and perform optimization tasks"));

    ui->actionGenerate_Password->setStatusTip(tr("Generate a new secure password"));
    ui->actionRandom_Noise->setStatusTip(tr("Generate random data in various formats"));
    ui->actionEncrypt_File->setStatusTip(tr("Encrypt a file using a password"));
    ui->actionDecrypt_File->setStatusTip(tr("Decrypt a password‑encrypted file"));
    ui->actionEncrypt_message->setStatusTip(tr("Encrypt a message using a password"));
    ui->actionDecrypt_message->setStatusTip(tr("Decrypt a password‑encrypted message"));
    ui->actionKey_List->setStatusTip(tr("Manage the list of keys allowed to encrypt passwords"));
    ui->actionClear_GPG_Passphrase_Cache->setStatusTip(tr("Clear cached GPG passphrases"));
    ui->actionExported_Passwords->setStatusTip(tr("View passwords that remain unchanged since being exported"));
    ui->actionLast_Edited->setStatusTip(tr("Identify passwords that may be outdated or insecure"));
    ui->actionRecent->setStatusTip(tr("Show the most recently accessed passwords"));
    ui->actionPopular->setStatusTip(tr("Show the most frequently accessed passwords"));

    ui->actionOnline_Documentation->setStatusTip(tr("Open the user guide in your web browser"));
    ui->actionSystem_Information->setStatusTip(tr("View detailed system and application information"));
    ui->actionDonate->setStatusTip(tr("Learn how to support the project"));
    ui->actionAbout->setStatusTip(tr("View information about this application"));
    ui->actionAbout_Qt->setStatusTip(tr("View information about the Qt toolkit"));

    ui->actionNew_Category->setStatusTip(tr("Create a new category for organizing passwords"));
    ui->actionNew_Password->setStatusTip(tr("Create a new password entry"));
    ui->actionDelete_Category->setStatusTip(tr("Delete the selected category"));
    ui->actionRename->setStatusTip(tr("Rename the selected category"));
    ui->actionImport->setStatusTip(tr("Import passwords from a JSON file into the selected category"));
    ui->actionRefresh_Categories->setStatusTip(tr("Reload the current view from the database"));
    ui->actionProperties->setStatusTip(tr("View statistics and details for the selected category"));

    ui->actionOpen_Password->setStatusTip(tr("Decrypt and display the selected password"));
    ui->actionEdit_Password->setStatusTip(tr("Decrypt and edit the selected password"));
    ui->actionExport_Password->setStatusTip(tr("Export the selected password as a JSON file"));
    ui->actionAdd_Search->setStatusTip(tr("Add additional search keywords for this password"));
    ui->actionDelete_Password->setStatusTip(tr("Delete the selected password permanently"));
    ui->actionBookmark->setStatusTip(tr("Add or remove this password from your bookmarks"));
    ui->actionAudit_Log->setStatusTip(tr("View the full activity history for this password"));


    // OTP countdown widgets (progress bar + label)
    countdownProgress = new QProgressBar(this);
    countdownProgress->setObjectName("progressTOTP");
    countdownProgress->setRange(0, 30);
    countdownProgress->setValue(30);
    countdownProgress->setFixedHeight(ui->statusbar->height() - 14);
    countdownProgress->setFixedWidth(100);
    countdownProgress->setVisible(false);
    countdownProgress->setTextVisible(false);
    countdownLabel = new QLabel(this);
    QFont fixedFont("Monospace");
    fixedFont.setStyleHint(QFont::TypeWriter);
    countdownLabel->setFont(fixedFont);
    countdownLabel->setVisible(false);
    ui->statusbar->addPermanentWidget(countdownProgress);
    ui->statusbar->addPermanentWidget(countdownLabel);


    //Signals and slots
    autoCloseTimer = new QTimer(this);
    autoCloseTimer->setSingleShot(true);

    //auto close password timer
    connect(autoCloseTimer,
            &QTimer::timeout,
            this,
            &MainWindow::clearScrollArea);

    //open database
    connect(ui->actionOpen_Database,
            &QAction::triggered,
            this,
            [this]() {
                const QString fileName = QFileDialog::getOpenFileName(
                    this,
                    ui->actionOpen_Database->text(),
                    QString(),
                    tr("Password Database (*.pwd)")
                );

                if (!fileName.isEmpty()) {
                    openDatabase(fileName);
                }
            });


    //maintenance of database
    connect(ui->actionMaintenance,
            &QAction::triggered,
            this,
            [this]() {
                // Create/open database here
                QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
                db.setDatabaseName(qApp->property("dbFile").toString());

                if (!db.open()) {
                    qWarning() << "Failed to open database";
                    return;  // just leave the lambda, don't try to show the dialog
                }

                DbMaintenance* dlg = new DbMaintenance(db, this);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->resize(500, 400);
                dlg->show();
            });



    //new password
    connect(ui->actionNew_Password,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget->currentItem()) {
                    newPassword();
                }
            });


    //open password
    connect(ui->actionOpen_Password,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    openPassword(item);
                }
            });


    //edit password
    connect(ui->actionEdit_Password,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    editPassword(item);
                }
            });

    //export password
    connect(ui->actionExport_Password,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    exportPassword(item);
                }
            });

    //audit log
    connect(ui->actionAudit_Log,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    showAuditLog(item);
                }
            });

    //new category
    connect(ui->actionNew_Category,
            &QAction::triggered,
            this,
            [this]() {
                createCategory();  // calls with default QString()
            });

    //rename category
    connect(ui->actionRename, &QAction::triggered,
            this, &MainWindow::renameCategory);

    //delete category
    connect(ui->actionDelete_Category,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget->currentItem()) {

                    deleteCategory(item);
                }
            });

    //refresh categories
    connect(ui->actionRefresh_Categories, &QAction::triggered,
            this, &MainWindow::loadCategories);

    //catgory properties
    connect(ui->actionProperties, &QAction::triggered, this, [this]() {

        QTreeWidgetItem* item = ui->treeWidget->currentItem();
        if (!item)
            return;

        int categoryId = item->data(0, Qt::UserRole).toInt();

        CategoryProperties dlg(categoryId, this);
        dlg.exec();
    });

    //import password (into category)
    connect(ui->actionImport, &QAction::triggered,
            this, [this]() {

                QString fileName = QFileDialog::getOpenFileName(
                    this,
                    tr("Select JSON File to Import"),
                    QDir::homePath(),
                    tr("JSON Files (*.json)")
                    );

                if (fileName.isEmpty()) {
                    return; // user cancelled
                }

                importApplicationsFromFile(fileName);
            });

    //generate password
    connect(ui->actionGenerate_Password,
            &QAction::triggered,
            this,
            [this]() {
                PasswordDialog::showPasswordGenerator(
                    this,
                    ui->actionGenerate_Password->text(),
                    QStringList()
                );
            });

    //generate random noise
    connect(ui->actionRandom_Noise,
            &QAction::triggered,
            this,
            [this]() {
                RandomNoiseDialog::showRandomNoiseGenerator(this);
            });

    //encrypt file
    connect(ui->actionEncrypt_File, &QAction::triggered,
            this, &MainWindow::encryptFile);

    //decrypt file
    connect(ui->actionDecrypt_File, &QAction::triggered,
            this, &MainWindow::decryptFile);

    //add search term
    connect(ui->actionAdd_Search,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    addSearchTerms(item);
                } else {
                    QMessageBox::warning(
                        this,
                        ui->actionAdd_Search->text(),
                        tr("No application selected.")
                    );
                }
            });

    //key List
    connect(ui->actionKey_List, &QAction::triggered,
            this, &MainWindow::keyList);

    //delete password
    connect(ui->actionDelete_Password,
            &QAction::triggered,
            this,
            [this]() {
                if (auto *item = ui->treeWidget_2->currentItem()) {
                    deletePassword(item);
                } else {
                    QMessageBox::warning(
                        this,
                        ui->actionDelete_Password->text(),
                        tr("No password selected.")
                    );
                }
            });

    //clear gpg agent cache
    connect(ui->actionClear_GPG_Passphrase_Cache,
            &QAction::triggered,
            this,
            [this]() {
                const bool ok = killGpgAgent();
                if (ok) {
                    QMessageBox::information(
                        this,
                        tr("GPG Agent"),
                        tr("Passphrase cache cleared.")
                    );
                } else {
                    QMessageBox::warning(
                        this,
                        tr("GPG Agent"),
                        tr("Failed to clear passphrase cache.")
                    );
                }
            });

    // Line edit search
    connect(ui->lineEditSearch, &QLineEdit::returnPressed,
            this, [this]() {
                search(ui->lineEditSearch->text().trimmed());
            });

    // Tree widget custom signals
    connect(ui->treeWidget, &WatermarkedTreeWidget::itemDropped,
            this, &MainWindow::moveCategory);

    // just-in-time loading bookmarks
    connect(ui->menuBookmarks, &QMenu::aboutToShow,
            this, &MainWindow::populateBookmarksMenu);

    //user guide
    connect(ui->actionOnline_Documentation, &QAction::triggered, this, [this]() {
        checkHelpReachable([this](bool reachable) {
            if (reachable) {
                const QUrl url(Passwords::HelpBaseUrl);
                QDesktopServices::openUrl(url);
            } else {
                launchHelperProcess(QStringLiteral(""));
            }
        });
    });

    //donate
    connect(ui->actionDonate, &QAction::triggered, this, [this]() {
        checkHelpReachable([this](bool reachable) {
            if (reachable) {
                const QUrl url(Passwords::HelpBaseUrl + QStringLiteral("donate"));
                QDesktopServices::openUrl(url);
            } else {
                launchHelperProcess(QStringLiteral("donate"));
            }
        });
    });

    //system information
    connect(ui->actionSystem_Information, &QAction::triggered,
            this, [this]() {
                SystemInfoDialog dlg(this);
                dlg.exec();
            });

    //about Qt
    connect(ui->actionAbout_Qt, &QAction::triggered,
            qApp, &QApplication::aboutQt);

    //about Passwords
    connect(ui->actionAbout, &QAction::triggered, this, [this] {
        AboutDialog dlg(this);
        dlg.exec();
    });

    //close (exit)
    connect(closeAction, &QAction::triggered,
            this, &MainWindow::close);

    connect(ui->treeWidget_2,
            &QTreeWidget::customContextMenuRequested,
            this,
            &MainWindow::showPasswordsContextMenu);


    if (settings.openCategoryDblClick())
    {
    connect(ui->treeWidget,
            &QTreeWidget::itemActivated,
            this,
            &MainWindow::openCategory);
    }
    else
    {

    connect(ui->treeWidget,
            &QTreeWidget::itemClicked,
            this,
            &MainWindow::openCategory);

    // Add arrow‑key / focus‑change behavior
    connect(ui->treeWidget,
            &QTreeWidget::currentItemChanged,
            this,
            &MainWindow::openCategoryFromCurrent);
    }



    connect(ui->actionBookmark, &QAction::toggled,
            this, [this](bool checked) {
                setBookmark(checked);
            });

    connect(ui->actionPreferences, &QAction::triggered,
            this, [this]() {

                QFileInfo fi(settings.configFilePath());
                if (!fi.isWritable()) {
                    qInfo() << "Config file is not writable:" << settings.configFilePath();
                    QMessageBox::warning(this,
                                         ui->actionPreferences->text(),
                                         "You are not authorised to make configuration changes.");
                    return;
                }

                auto *pd = new PreferencesDialog(this);
                pd->setWindowTitle(ui->actionPreferences->text());
                pd->adjustSize();
                pd->setFixedSize(pd->size());
                pd->setWindowFlags(pd->windowFlags() & ~Qt::WindowMaximizeButtonHint);
                pd->exec();
            });

    connect(ui->treeWidget_2,
            &QTreeWidget::itemActivated,
            this,
            &MainWindow::openPassword);

    connect(ui->actionRecent, &QAction::triggered,
            this, &MainWindow::searchRecent);

    connect(ui->actionPopular, &QAction::triggered,
            this, &MainWindow::searchPopular);


    connect(ui->actionEncrypt_message, &QAction::triggered,
            this, &MainWindow::encryptMessage);

    connect(ui->actionDecrypt_message, &QAction::triggered,
            this, &MainWindow::decryptMessage);

    connect(ui->actionExported_Passwords, &QAction::triggered,
            this, &MainWindow::ExportedWithoutEdits);

    //report: passwords older than N
    connect(ui->actionLast_Edited, &QAction::triggered, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Select Cutoff Date"));

        auto *layout = new QVBoxLayout(&dlg);

        auto *label = new QLabel(tr("Show passwords not edited since:"), &dlg);
        layout->addWidget(label);

        auto *dateEdit = new QDateEdit(&dlg);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDate(QDate::currentDate().addDays(-90)); // default 90 days
        layout->addWidget(dateEdit);

        auto *btnLayout = new QHBoxLayout();
        auto *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
        auto *okBtn = new QPushButton(tr("OK"), &dlg);

        btnLayout->addStretch();
        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(okBtn);
        layout->addLayout(btnLayout);

        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            const QDateTime cutoff = dateEdit->date().startOfDay();
            NotChangedSince(cutoff);
        }
    });

    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
            this, [this](const QPoint &pos)
            {
                QMenu menu;
                QPoint globalPos = ui->treeWidget->viewport()->mapToGlobal(pos);

                const bool hasDb = !qApp->property("dbFile").toString().trimmed().isEmpty();
                const bool hasSelection = !ui->treeWidget->selectedItems().isEmpty();

                if (!hasSelection)
                {
                    if (hasDb)
                    {
                        menu.addAction(ui->actionNew_Category);
                    }
                    else
                    {
                        QAction *d = new QAction("No database open", &menu);
                        d->setEnabled(false);
                        menu.addAction(d);
                    }
                }
                else
                {
                    if (hasDb)
                    {
                        menu.addAction(ui->actionNew_Password);
                        menu.addAction(ui->actionNew_Category);
                        menu.addSeparator();

                        menu.addAction(ui->actionRename);
                        menu.addAction(ui->actionDelete_Category);
                        menu.addSeparator();

                        menu.addAction(ui->actionRefresh_Categories);
                        menu.addAction(ui->actionImport);
                        menu.addSeparator();

                        menu.addAction(ui->actionProperties);
                    }
                    else
                    {
                        QAction *d = new QAction("No database open", &menu);
                        d->setEnabled(false);
                        menu.addAction(d);
                    }
                }

                menu.exec(globalPos);
            });
    // ui->treeWidget_2->setStyleSheet(
    //     "QTreeWidget::item { padding-top: 6px; padding-bottom: 6px; }"
    //     );

    setupDebugWarnings(this, ui->statusbar);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (abortingStartup) {
        event->accept();
        return;
    }

    if (settings.getAskClose()) {
        auto reply = QMessageBox::question(this,
                                           tr("Confirm Exit"),
                                           tr("Do you really want to quit?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    // Delete unnecessary backups
    const QString dbPath = qApp->property("dbFile").toString();
    QFileInfo dbInfo(dbPath);

    if (dbInfo.exists()) {
        const QString backupDir = dbInfo.absolutePath() + "/backups";

        if (QDir(backupDir).exists()) {
            manageBackups(backupDir,
                          settings.getMaxBackups(),
                          this);   // parent for optional progress dialog
        }
    }

    // Close local help server
    if (settings.getCloseHelpServer()) {
        if (helperProcess) {
            // Disconnect error handler so no spurious pop)up
            disconnect(helperProcess, &QProcess::errorOccurred, nullptr, nullptr);
            if (helperProcess->state() != QProcess::NotRunning) {
                helperProcess->terminate();
                if (!helperProcess->waitForFinished(2000)) {
                    helperProcess->kill();
                }
                qInfo().noquote() << Q_FUNC_INFO << "Closed help server";
            }
        } else
        {
            qInfo().noquote() << "Could not find a help server to close.";
        }
    }

    if (settings.getKillGpgAgent("KillGPGAgentOnExit"))
    {
        qInfo().noquote() << "Killing GPG Agent:" << killGpgAgent();
    }

    if (!qApp->property("skipSave").toBool()) {
        settings.saveMainWindowState(this);
        settings.saveSplitterState(vSplitter, "vsplit");
        settings.saveSplitterState(hSplitter, "hsplit");
    }

    // Update last used file
    settings.setLastUsedFile(qApp->property("dbFile").toString());

    event->accept();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (qApp->property("dbFile").toString().isEmpty())
        return;

    if (firstShow) {
        firstShow = false;

        QMetaObject::invokeMethod(this, [this]() {

            QTimer::singleShot(250, this, [this]() {

                int count = countExportedWithoutEdits();
                if (count <= 0)
                    return;

                QString plural = (count == 1 ? "" : "s");
                bool singular = (count == 1);

                QString message = QString(
                    "There %1 %2 exported password%3 that %4 not been updated since %5 last exported.\n\n"
                    "For security reasons, %6 password%3 should be updated or removed from the database.\n\n"
                    "Would you like to review %7 now?"
                )
                .arg(singular ? "is" : "are")        // %1
                .arg(count)                          // %2
                .arg(plural)                         // %3
                .arg(singular ? "has" : "have")      // %4
                .arg(singular ? "it was" : "they were") // %5
                .arg(singular ? "this" : "these")    // %6
                .arg(singular ? "it" : "them");      // %7


                QMessageBox::StandardButton reply =
                    QMessageBox::warning(
                        this,
                        "Security Notice",
                        message,
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::Yes
                        );

                if (reply == QMessageBox::Yes) {
                    ExportedWithoutEdits();
                }

            });

        }, Qt::QueuedConnection);
    }
}
void MainWindow::loadCategories()
{
    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            return;
        }

        // Ensure foreign keys are enforced
        QSqlQuery pragma(db);
        pragma.exec("PRAGMA foreign_keys = ON");

        ui->treeWidget->blockSignals(true);
        ui->treeWidget->clear();

        QMap<int, QTreeWidgetItem*> itemMap;
        QVector<QPair<int,int>> links; // (id, parentId)

        // First pass: create all items and add them as top-level
        {
            QSqlQuery query(db);
            if (!query.exec("SELECT id, parent_id, text FROM categories ORDER BY id")) {
                showQueryError(this,query,Q_FUNC_INFO);
                return;
            }

            while (query.next()) {
                int id = query.value(0).toInt();
                int parentId = query.value(1).isNull() ? 0 : query.value(1).toInt();
                QString text = DataObfuscator::deobfuscate(query.value(2).toString(), appKey);
                qDebug() << id << parentId << text;

                QTreeWidgetItem* item = new QTreeWidgetItem();
                item->setText(0, text);
                item->setData(0, Qt::UserRole, id);
                item->setIcon(0, closedIcon);

                itemMap[id] = item;
                ui->treeWidget->addTopLevelItem(item);
                links.push_back({id, parentId});
            }
        }

        // Helper: detect if 'ancestor' is an ancestor of 'node'
        auto isAncestorOf = [](QTreeWidgetItem* ancestor, QTreeWidgetItem* node) {
            for (QTreeWidgetItem* p = node->parent(); p != nullptr; p = p->parent()) {
                if (p == ancestor) return true;
            }
            return false;
        };

        // Second pass: attempt safe re-parenting
        for (const auto& link : links) {
            int id = link.first;
            int parentId = link.second;

            QTreeWidgetItem* item   = itemMap.value(id, nullptr);
            if (!item) continue;

            if (parentId == 0) continue; // no parent

            QTreeWidgetItem* parent = itemMap.value(parentId, nullptr);
            if (!parent) {
                qWarning().noquote() << Q_FUNC_INFO << "Orphan category id" << id << "parent" << parentId << "not found; leaving as top-level.";
                continue;
            }

            if (parentId == id) {
                qWarning().noquote() << Q_FUNC_INFO << "Self-parent detected for id" << id << "; skipping re-parent.";
                continue;
            }

            if (isAncestorOf(item, parent)) {
                qWarning().noquote() << Q_FUNC_INFO << "Cycle detected while linking id" << id << "-> parent" << parentId << "; skipping.";
                continue;
            }

            // Move from top-level to parent
            if (item->parent()) {
                item->parent()->removeChild(item);
            } else {
                int idx = ui->treeWidget->indexOfTopLevelItem(item);
                if (idx >= 0) ui->treeWidget->takeTopLevelItem(idx);
            }
            parent->addChild(item);
        }

        ui->treeWidget->blockSignals(false);

        // Reconnect expand/collapse handlers
        QObject::disconnect(ui->treeWidget, &QTreeWidget::itemExpanded, nullptr, nullptr);
        QObject::disconnect(ui->treeWidget, &QTreeWidget::itemCollapsed, nullptr, nullptr);

        connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, [=](QTreeWidgetItem *item){
            if (item->childCount() > 0) {
                item->setIcon(0, QIcon(":/menus/glyphs/folder_open_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
            }
        });
        connect(ui->treeWidget, &QTreeWidget::itemCollapsed, this, [=](QTreeWidgetItem *item){
            if (item->childCount() > 0) {
                item->setIcon(0, QIcon(":/menus/glyphs/folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
            }
        });

        // Select first item
        if (ui->treeWidget->topLevelItemCount() > 0) {
            QTreeWidgetItem *firstItem = ui->treeWidget->topLevelItem(0);
            ui->treeWidget->setCurrentItem(firstItem);
            firstItem->setSelected(true);
            openCategory(firstItem, 0);
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newPassword()
{
    NewPasswordDialog dlg(this);
    dlg.setWindowTitle(ui->actionNew_Password->text());

    QList<KeyEntry> keys = fetchKeys();
    dlg.setKeys(keys);

    if (dlg.exec() == QDialog::Accepted) {
        QStringList selectedKeys = dlg.getCheckedKeys();

        QString baseDir = "/dev/shm";
        if (!QFileInfo::exists(baseDir) || !QFileInfo(baseDir).isWritable()) {
            baseDir = QDir::tempPath();
        }

        QString tempFile = baseDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".asc";


        QStringList args;
        for (const QString &key : std::as_const(selectedKeys)) {
            args << "--recipient" << key;
        }
        args << "--encrypt" << "--armor" << "--output" << tempFile;

        qInfo().noquote() << "Running command:" << "gpg" << args.join(' ');

        QProcess process;
        process.start("gpg", args);
        if (!process.waitForStarted()) {
            qDebug().noquote() << "Failed to start GPG process.";
            QMessageBox::critical(this,ui->actionNew_Password->text(),tr("Failed to start GPG process."));
            return;
        }

        QByteArray jsonData = dlg.toJson();
        process.write(jsonData);
        process.waitForBytesWritten();
        process.closeWriteChannel();

        if (!process.waitForFinished()) {
            QMessageBox::critical(this,ui->actionNew_Password->text(),tr("GPG process did not finish correctly."));
            qCritical().noquote() << Q_FUNC_INFO << "GPG process did not finish correctly.";
            return;
        }

        QByteArray errors = process.readAllStandardError();
        if (!errors.isEmpty()) {
            qCritical().noquote() << Q_FUNC_INFO << "GPG Errors:\n" << QString::fromUtf8(errors);
            QMessageBox::critical(this,ui->actionNew_Password->text(),QString::fromUtf8(errors));
            return;
        }

        QFile outFile(tempFile);
        if (!outFile.exists() || !outFile.open(QIODevice::ReadOnly)) {
            qCritical().noquote() << Q_FUNC_INFO << "Encrypted file not found or failed to open:" << tempFile;
            wipeFile(tempFile);
            return;
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","sqlNewPassword");

            int categoryId = ui->treeWidget->currentItem()->data(0,Qt::UserRole).toInt();
            QString publicAppName = dlg.PublicAppName;
            QByteArray data = outFile.readAll();

            db.setDatabaseName(qApp->property("dbFile").toString());
            if (!db.open()) {
                showDbNotOpenError(this, db, Q_FUNC_INFO);
                return;
            }

            // --- Transaction start ---
            if (!db.transaction()) {
                showTransactionError(this,db,Q_FUNC_INFO);
                return;
            }
            bool success = true;

            // Insert application
            QSqlQuery query(db);
            query.prepare(R"(
                INSERT INTO application (category_id, application_name, data, created)
                VALUES (:category_id, :application_name, :data, :created))");

            query.bindValue(":category_id", categoryId);
            query.bindValue(":application_name", DataObfuscator::obfuscate(publicAppName,this->appKey));
            query.bindValue(":data", DataObfuscator::obfuscate(QString::fromUtf8(data),appKey));
            query.bindValue(":created", QDateTime::currentSecsSinceEpoch());

            if (!query.exec()) {
                showQueryError(this,query,Q_FUNC_INFO);
                qCritical().noquote() << Q_FUNC_INFO << "Insert failed:" << query.lastError().text();
                success = false;
            }

            int appId = query.lastInsertId().toInt();
            qDebug() << "last insert id" << appId;

            // Tokenize and hash search terms
            if (success) {
                // Normalize: lowercase, replace any non-letter/digit with a space
                QString normalized = publicAppName.toLower();
                static const QRegularExpression nonWordChars("[^\\p{L}\\p{N}]+");
                normalized.replace(nonWordChars, " ");
                normalized = normalized.trimmed();

                // Split on whitespace
                static const QRegularExpression whitespaceRe("\\s+");
                QStringList tokens = normalized.split(whitespaceRe, Qt::SkipEmptyParts);

                for (const QString &token : std::as_const(tokens)) {
                    QByteArray hash = QCryptographicHash::hash(token.toUtf8(),
                                                               QCryptographicHash::Sha256).toHex();
                    QSqlQuery insertToken(db);
                    insertToken.prepare("INSERT INTO application_tokens (application_id, token_hash) VALUES (?, ?)");
                    insertToken.addBindValue(appId);
                    insertToken.addBindValue(QString(hash));
                    if (!insertToken.exec()) {
                        qCritical().noquote() << Q_FUNC_INFO << "Token insert failed:" << insertToken.lastError().text();
                        success = false;
                        break;
                    }
                }
            }

            // Commit or rollback
            if (success) {
                if (!db.commit()) {
                    qCritical().noquote() << Q_FUNC_INFO  << "Commit failed:" << db.lastError().text();
                    db.rollback();
                } else {
                    qDebug() << "Transaction committed successfully.";

                    // Insert CREATED audit row
                        insertAuditRow(appId,
                                       userName,
                                       QSysInfo::machineHostName(),
                                       "CREATED");

                    if (!ui->treeWidget->selectedItems().isEmpty()) {
                        openCategory(ui->treeWidget->selectedItems().first(), 0);
                    }
                }
            } else {
                db.rollback();
                qCritical().noquote() << Q_FUNC_INFO << "Transaction rolled back due to errors.";
            }

            outFile.close();
            outFile.remove();

        }
        QSqlDatabase::removeDatabase("sqlNewPassword");
    }
}


QList<KeyEntry> MainWindow::fetchKeys() const
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<KeyEntry> keys;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open())
        {
            QSqlQuery query(db);
            query.setForwardOnly(true);
            if (query.exec("SELECT id, label, key FROM keys"))
            {
            while (query.next()) {
                KeyEntry entry;
                entry.id    = query.value(0).toInt();
                entry.label = DataObfuscator::deobfuscate(query.value(1).toString(),qApp->property("appKey").toByteArray());
                entry.key   = DataObfuscator::deobfuscate(query.value(2).toString(),qApp->property("appKey").toByteArray());
                keys.append(entry);
            }
            } else
            {
                showQueryError(this,query,Q_FUNC_INFO);
            }
        } else
        {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return keys;
}

void MainWindow::openCategory(QTreeWidgetItem *item, int column) {
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    clearScrollArea();

    // Block signals and clear the second tree widget
    ui->treeWidget_2->blockSignals(true);
    ui->treeWidget_2->clear();

    // Create the database connection
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            ui->treeWidget_2->blockSignals(false); // Unblock signals on error
            return;
        }

        // Prepare and execute the query
        QSqlQuery query(db);
        query.prepare(QString("SELECT * FROM application WHERE category_id = :id ORDER BY id"));
        query.bindValue(":id", item->data(0,Qt::UserRole).toInt());

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            db.close();
            QSqlDatabase::removeDatabase(connName);
            ui->treeWidget_2->blockSignals(false);
            return;
        }

        // Process each row of the query result and create tree items
        while (query.next()) {
            QTreeWidgetItem* newItem = nullptr;
            newItem = makeItemFromApplication(query);
            if (newItem) {
                ui->treeWidget_2->addTopLevelItem(newItem);
            }
        }

        // Clean up
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    // Unblock signals after processing is complete
    ui->treeWidget_2->blockSignals(false);
    ui->treeWidget_2->clearSelection();
    ui->treeWidget_2->setCurrentItem(nullptr);

    // Reset openedCredentialID
    openedCredentialID = -1;
}

void MainWindow::clearScrollArea()
{
    if (countdownTimer) {
        countdownTimer->stop();
        delete countdownTimer;
        countdownTimer = nullptr;
    }

    if (alignedTimer) {
        alignedTimer->stop();
        delete alignedTimer;
        alignedTimer = nullptr;
    }

    // Hide countdown label and progress
    countdownLabel->setVisible(false);
    countdownProgress->setVisible(false);

    QWidget *container = ui->scrollArea->widget();
    if (!container) {
        // If no container yet, create one and attach it
        container = new QWidget;
        ui->scrollArea->setWidget(container);
        ui->scrollArea->setWidgetResizable(true);
        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    // Ensure the container has a layout
    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(container->layout());
    if (!gridLayout) {
        gridLayout = new QGridLayout(container);
        gridLayout->setContentsMargins(12, 12, 12, 12);
        gridLayout->setSpacing(8);
    }

    // Clear existing widgets from the layout
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget(); // deletes any existing DropLabel
        }
        delete child;
    }

    // Add a fresh DropLabel
    DropLabel *imageLabel = new DropLabel(container);
    QPixmap pixmap(":/place.png");
    imageLabel->setPixmap(pixmap);
    imageLabel->setScaledContents(true);
    imageLabel->setFixedSize(275, 105);

    gridLayout->addWidget(imageLabel, 0, 0);

    connect(imageLabel, &DropLabel::itemDropped,
            this, [this](QTreeWidgetItem *item) {
                if (!item) return;
                if (settings.getKillGpgAgent()) {
                    killGpgAgent();
                }
                openPassword(item);
            });

    openedCredentialID = -1;
}

// void MainWindow::openPassword(QTreeWidgetItem *item)
// {
//     if (settings.getKillGpgAgent()) {
//         killGpgAgent();
//     }

//     int parentId = item->data(0, Qt::UserRole).toInt();

//     if (openedCredentialID == parentId)
//     {
//         clearScrollArea();
//         return;
//     }

//     QApplication::setOverrideCursor(Qt::WaitCursor);
//     QApplication::processEvents();

//     QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

//     // --- Step 0: Map mode to table/column names ---
//     QString viewTable, idColumn, credTable;

//     viewTable      = "application_views";
//     idColumn       = "application_id";
//     credTable      = "application";

//     // --- Step 1: Retrieve encrypted data ---
//     ui->statusbar->showMessage("Reading database..");
//     QApplication::processEvents();

//     QByteArray data;
//     {
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//         db.setDatabaseName(qApp->property("dbFile").toString());
//         if (db.open()) {
//             QSqlQuery query(db);
//             query.prepare(QString("SELECT data FROM application WHERE id = :id"));
//             query.bindValue(":id", parentId);
//             if (query.exec() && query.first()) {
//                 data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
//             } else {
//                 showQueryError(this,query,Q_FUNC_INFO);
//             }
//         } else {
//             showDbNotOpenError(this, db, Q_FUNC_INFO);
//             QApplication::restoreOverrideCursor();
//             QApplication::processEvents();
//             return;
//         }
//     }
//     QSqlDatabase::removeDatabase(connName);
//     ui->statusbar->clearMessage();

//     // --- Step 2: Decrypt the data (async, non-blocking) ---
//     ui->statusbar->showMessage("Decrypting data..");
//     QApplication::processEvents();

//     QProcess* gpg = new QProcess(this);
//     gpg->setProcessChannelMode(QProcess::SeparateChannels);

//     connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
//         gpg->write(data);
//         data.fill(0);
//         gpg->closeWriteChannel();
//     });

//     // Handle stdout (decrypted JSON)
//     connect(gpg, &QProcess::readyReadStandardOutput, this, [this, gpg, item]() {
//         QByteArray decrypted_data = gpg->readAllStandardOutput();
//         if (!decrypted_data.isEmpty()) {
//             ui->statusbar->clearMessage();
//             populateFromJsonApplication(decrypted_data, ui);
//             decrypted_data.fill(0);
//         }
//     });

//     // Handle stderr
//     bool *noKeyShown = new bool(false);

//     connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg, noKeyShown]() {
//         QByteArray errors = gpg->readAllStandardError();
//         if (!errors.isEmpty()) {

//             QString errStr = QString::fromUtf8(errors);

//             ui->statusbar->showMessage(errStr);
//             qDebug().noquote() << "GPG stderr:" << errors;

//             if (errStr.contains("no secret key", Qt::CaseInsensitive) && !*noKeyShown) {
//                 *noKeyShown = true;
//                 QMessageBox::critical(
//                             this,
//                             tr("GPG Error"),
//                             errStr
//                             );
//             }
//         }
//     });

//     // --- Step 4: Handle completion ---
//     connect(gpg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//             this, [this, gpg, parentId, connName, viewTable, idColumn, noKeyShown]
//             (int exitCode, QProcess::ExitStatus status) {

//         delete noKeyShown;
//         if (status == QProcess::NormalExit && exitCode == 0) {
//             openedCredentialID = parentId;

//             // Upsert into views table only after successful decryption
//             {
//                 QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//                 db.setDatabaseName(qApp->property("dbFile").toString());
//                 if (db.open()) {
//                     QSqlQuery upsert(db);
//                     upsert.prepare(QString(
//                                        "INSERT INTO %1(%2, view_count, dt, user, host) "
//                                        "VALUES (:id, 1, :ts, :user, :host) "
//                                        "ON CONFLICT(%2) DO UPDATE SET "
//                                        "view_count = view_count + 1, dt = :ts, user = :user, host = :host"
//                                        ).arg(viewTable, idColumn));

//                     upsert.bindValue(":id", parentId);
//                     upsert.bindValue(":ts", QDateTime::currentSecsSinceEpoch());
//                     upsert.bindValue(":user", userName);
//                     upsert.bindValue(":host", QSysInfo::machineHostName());
//                     if (!upsert.exec()) {
//                         showQueryError(this,upsert,Q_FUNC_INFO);
//                     }
//                 } else
//                 {
//                     showDbNotOpenError(this, db, Q_FUNC_INFO);
//                     QApplication::restoreOverrideCursor();
//                     QApplication::processEvents();
//                     return;
//                 }
//             }
//             QSqlDatabase::removeDatabase(connName);

//         }
//         gpg->deleteLater();
//         QApplication::restoreOverrideCursor();
//         QApplication::processEvents();
//     });

//     gpg->start("gpg", QStringList() << "--decrypt");
// }


void MainWindow::openPassword(QTreeWidgetItem *item)
{
    if (settings.getKillGpgAgent()) {
        killGpgAgent();
    }

    int parentId = item->data(0, Qt::UserRole).toInt();

    if (openedCredentialID == parentId) {
        clearScrollArea();
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // --- Step 0: Map mode to table/column names ---
    QString viewTable      = "application_views";
    QString idColumn       = "application_id";
    QString credTable      = "application";

    // --- Step 1: Retrieve encrypted data ---
    ui->statusbar->showMessage("Reading database..");
    QApplication::processEvents();

    QByteArray encdata;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT data FROM application WHERE id = :id");
            query.bindValue(":id", parentId);

            if (query.exec() && query.first()) {
                encdata = DataObfuscator::deobfuscate(
                              query.value(0).toString(), appKey
                          ).toUtf8();
            } else {
                showQueryError(this, query, Q_FUNC_INFO);
            }
        } else {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
            return;
        }
    }

    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt using reusable helper ---
    ui->statusbar->showMessage("Decrypting data..");
    QApplication::processEvents();

    decryptWithGpg(
        encdata,

        // --- onSuccess ---
        [this, parentId, connName, viewTable, idColumn](const QByteArray &json) {
            ui->statusbar->clearMessage();
            populateFromJsonApplication(json, ui);

            // Mark as opened
            openedCredentialID = parentId;

            // Upsert into views table
            {
                QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
                db.setDatabaseName(qApp->property("dbFile").toString());

                if (db.open()) {
                    QSqlQuery upsert(db);
                    upsert.prepare(QString(
                        "INSERT INTO %1(%2, view_count, dt, user, host) "
                        "VALUES (:id, 1, :ts, :user, :host) "
                        "ON CONFLICT(%2) DO UPDATE SET "
                        "view_count = view_count + 1, dt = :ts, user = :user, host = :host"
                    ).arg(viewTable, idColumn));

                    upsert.bindValue(":id", parentId);
                    upsert.bindValue(":ts", QDateTime::currentSecsSinceEpoch());
                    upsert.bindValue(":user", userName);
                    upsert.bindValue(":host", QSysInfo::machineHostName());

                    if (!upsert.exec()) {
                        showQueryError(this, upsert, Q_FUNC_INFO);
                    }
                } else {
                    showDbNotOpenError(this, db, Q_FUNC_INFO);
                }
            }

            QSqlDatabase::removeDatabase(connName);
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
        },

        // --- onMissingKey ---
        [this](const QString &err) {
            ui->statusbar->clearMessage();
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
            QMessageBox::critical(this, tr("GPG Error"), err);
            clearScrollArea();
        },

        // --- onFailure ---
        [this](const QString &err) {
            ui->statusbar->showMessage(err);
            clearScrollArea();
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
        }
    );
}


void MainWindow::populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui) {

    // Ensure scrollArea has a widget
    if (!ui->scrollArea->widget()) {
        QWidget *container = new QWidget;
        ui->scrollArea->setWidget(container);
        ui->scrollArea->setWidgetResizable(true);
    }

    // Get or create grid layout
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollArea->widget()->layout());
    if (!gridLayout) {
        gridLayout = new QGridLayout(ui->scrollArea->widget());
        ui->scrollArea->widget()->setLayout(gridLayout);
    }

    // Set left and right margin to 0.
    int left, top, right, bottom;
    gridLayout->getContentsMargins(&left, &top, &right, &bottom);
    gridLayout->setContentsMargins(3, top, 3, bottom);


    // Clear existing widgets
    while (QLayoutItem* item = gridLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray credentials = root.value("credentials").toArray();

    // Add a full-width label in the next row for name/description
    QString name        = root.value("private_name").toString().trimmed();
    QString description = root.value("description").toString().trimmed();

    QString labelText;
    if (!name.isEmpty() && !description.isEmpty()) {
        if (name == description) {
            labelText = name;  // avoid duplicate
        } else {
            labelText = name + "\n" + description;
        }
    } else if (!name.isEmpty()) {
        labelText = name;
    } else if (!description.isEmpty()) {
        labelText = description;
    } else {
        labelText = "(No name)"; // nothing to show
    }

    int row = 0;

    QLabel* fullWidthLabelName = new QLabel(labelText);
    fullWidthLabelName->setObjectName("labelNameHeading");
    gridLayout->addWidget(fullWidthLabelName, 0, 0, 1, -1); // span all columns
    row++;

    // Add a full-width label in the next row for url (optional)
    if (root.contains("url")) {
        QString url = root.value("url").toString().trimmed();

        if (!url.isEmpty()) {
            QLabel* fullWidthLabel_url = new QLabel("<a href=\"" + url + "\">" + url + "</a>");
            fullWidthLabel_url->setObjectName("labelPasswordURL");
            fullWidthLabel_url->setTextFormat(Qt::RichText);
            fullWidthLabel_url->setTextInteractionFlags(Qt::TextBrowserInteraction);
            fullWidthLabel_url->setOpenExternalLinks(true);

            gridLayout->addWidget(fullWidthLabel_url, row, 0, 1, -1);
            row++;
        }
    }

    for (const QJsonValue &val : std::as_const(credentials)) {
        QJsonObject credObj = val.toObject();
        QString username = credObj.value("username").toString();
        QString password = credObj.value("password").toString();
        QString otp      = credObj.value("secretOtpCode").toString();
        int otpLength    = credObj.value("length").toInt(6);
        qDebug() << "length " << otpLength;

        //
        // Username
        //
        QLabel* usernameLabel = new QLabel("Username");

        QLineEdit* usernameEdit = new QLineEdit();
        usernameEdit->setText(username);
        usernameEdit->setContextMenuPolicy(Qt::ActionsContextMenu);
        usernameEdit->setReadOnly(true);

        QAction *copyUsernameAction = new QAction(tr("Copy Username"), usernameEdit);
        copyUsernameAction->setIcon(QIcon(":/menus/glyphs/content_copy_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        copyUsernameAction->setStatusTip(tr("Username only remains in clipboard for 15 seconds"));
        usernameEdit->addAction(copyUsernameAction, QLineEdit::TrailingPosition);

        connect(copyUsernameAction, &QAction::triggered, this, [usernameEdit]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(usernameEdit->text());
            QTimer::singleShot(15000, qApp, [clipboard]() {
                clipboard->clear();
                clipboard->setText("");
                qDebug() << "clipboard cleared";
            });
        });

        //
        // Password
        //
        QLabel* passwordLabel = new QLabel("Password");

        QLineEdit* passwordEdit = new QLineEdit();
        passwordEdit->setText(password);
        passwordEdit->setContextMenuPolicy(Qt::ActionsContextMenu);
        passwordEdit->setEchoMode(settings.getEchoMode());
        passwordEdit->setReadOnly(true);

        QAction *showPasswordAction = new QAction(tr("Show Password"), passwordEdit);
        showPasswordAction->setCheckable(true);
        showPasswordAction->setStatusTip(tr("Show or hide the password text"));
        passwordEdit->addAction(showPasswordAction);

        connect(showPasswordAction, &QAction::toggled,
                this, [this, passwordEdit](bool checked) {
                    passwordEdit->setEchoMode(checked ? QLineEdit::Normal
                                                      : settings.getEchoMode());
                });

        QAction *inspectPasswordAction = new QAction(tr("Inspect Password"), this);
        inspectPasswordAction->setIcon(QIcon(":/menus/glyphs/preview_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        inspectPasswordAction->setStatusTip("Analyze this password character by character");
        passwordEdit->addAction(inspectPasswordAction, QLineEdit::TrailingPosition);

        connect(inspectPasswordAction, &QAction::triggered,
                this, [passwordEdit]() {
                    const QString password = passwordEdit->text();
                    QMetaObject::invokeMethod(
                        qApp,
                        [password]() {
                            auto *dlg = new PasswordDialog::PasswordInspectorDialog(password, nullptr);
                            dlg->setAttribute(Qt::WA_DeleteOnClose);
                            dlg->setWindowModality(Qt::NonModal);
                            dlg->show();
                        },
                        Qt::QueuedConnection);
                });

        QAction *copyPasswordAction = new QAction(tr("Copy Password"), passwordEdit);
        copyPasswordAction->setIcon(QIcon(":/menus/glyphs/content_copy_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        copyPasswordAction->setStatusTip("Password only remains in clipboard for 15 seconds");
        passwordEdit->addAction(copyPasswordAction, QLineEdit::TrailingPosition);

        connect(copyPasswordAction, &QAction::triggered, this, [passwordEdit]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(passwordEdit->text());
            QTimer::singleShot(15000, qApp, [clipboard]() {
                clipboard->clear();
                clipboard->setText("");
                qDebug() << "clipboard cleared";
            });
        });

        //
        // OTP
        //
        QLabel* otpLabel = new QLabel("OTP Code");

        QLineEdit* otpEdit = new QLineEdit();
        otpEdit->setContextMenuPolicy(Qt::ActionsContextMenu);
        otpEdit->setText(formatOtp(otp));
        otpEdit->setPlaceholderText(formatOtp(otp));
        otpEdit->setObjectName("otpEdit" + QString::number(row));
        otpEdit->setProperty("otpLength", otpLength);
        otpEdit->setReadOnly(true);

        QAction *copyOTPAction = new QAction(tr("Copy Code"), otpEdit);
        copyOTPAction->setIcon(QIcon(":/menus/glyphs/content_copy_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        copyOTPAction->setStatusTip(tr("Copy the TOTP code to your clipboard"));
        otpEdit->addAction(copyOTPAction, QLineEdit::TrailingPosition);

        connect(copyOTPAction, &QAction::triggered, this, [otpEdit]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(otpEdit->text().remove("-").trimmed());
        });

        //
        // Compact two-row layout for this credential
        //
        QWidget *credWidget = new QWidget();
        QGridLayout *credLayout = new QGridLayout(credWidget);

        credLayout->setContentsMargins(0, 0, 0, 0);
        credLayout->setHorizontalSpacing(12);
        credLayout->setVerticalSpacing(2);

        // Row 0: labels
        credLayout->addWidget(usernameLabel, 0, 0);
        credLayout->addWidget(passwordLabel, 0, 1);
        credLayout->addWidget(otpLabel,      0, 2);

        // Row 1: edits
        credLayout->addWidget(usernameEdit,  1, 0);
        credLayout->addWidget(passwordEdit,  1, 1);
        credLayout->addWidget(otpEdit,       1, 2);

        //
        // Insert into main grid
        //
        gridLayout->addWidget(credWidget, row, 0, 1, -1);

        row += 1;
    }

    gridLayout->setRowMinimumHeight(row, 32);
    row++;

    // --- Notes section ---
    if (root.contains("notes")) {
        QJsonArray notesArray = root.value("notes").toArray();

        if (!notesArray.isEmpty()) {

            // Section header (only created when notes exist)
            QLabel* notesHeader = new QLabel("Notes");
            notesHeader->setObjectName("labelNotesHeading");
            gridLayout->addWidget(notesHeader, row, 0, 1, -1);
            row++;

            // Each note as a full-width label
            for (const QJsonValue &noteVal : std::as_const(notesArray)) {
                QJsonObject noteObj = noteVal.toObject();
                QString content = noteObj.value("content").toString();

                QLabel* noteLabel = new QLabel(content);
                noteLabel->setWordWrap(true);
                noteLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

                gridLayout->addWidget(noteLabel, row, 0, 1, -1);
                row++;
            }

            gridLayout->setRowMinimumHeight(row, 32);
            row++;
        }
    }


    // Add a full-width label in the next row for Password/Object Details

    QLabel* fullWidthLabel = new QLabel("About this password");
    fullWidthLabel->setObjectName("labelAboutPasswordHeading");
    gridLayout->addWidget(fullWidthLabel, row, 0, 1, -1); // span all columns

    row++;

    /*
     * Now get the data for this application from the application_views table
     * This is where count and login information exists.
     */

    {
        // Create a named connection
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetchviews");
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
        } else {
            QList<QTreeWidgetItem*> selected = ui->treeWidget_2->selectedItems();
            if (!selected.isEmpty()) {
                QTreeWidgetItem *item = selected.first();
                int id = item->data(0,Qt::UserRole).toInt();

                {
                    QSqlQuery query(db);
                    query.prepare(
                        "SELECT "
                        "  (SELECT COUNT(*) FROM application_views_audit WHERE application_id = :id) AS view_count, "
                        "  a.dt AS dt, "
                        "  a.user AS user, "
                        "  a.host AS host "
                        "FROM application_views_audit a "
                        "WHERE a.application_id = :id "
                        "ORDER BY a.audit_id DESC "
                        "LIMIT 1"
                        );

                    query.bindValue(":id", id);

                    if (query.exec() && query.first()) {

                        QLocale locale = QLocale::system();

                        // Accessed count
                        QString accessedText = QString("Accessed: %1 times")
                                                   .arg(locale.toString(query.value(0).toLongLong()));

                        // Last accessed datetime
                        qint64 epoch = query.value(1).toLongLong();
                        QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch);
                        QString lastAccessedText = QString("Last access: %1")
                                                       .arg(locale.toString(dt, QLocale::ShortFormat));

                        // Last access by
                        QString lastAccessByText = QString("Last access by: %1 on %2")
                                                       .arg(query.value(2).toString(),
                                                            query.value(3).toString());

                        // Combine into one multi-line label
                        QString combined =
                            accessedText + "\n" +
                            lastAccessedText + "\n" +
                            lastAccessByText;

                        QLabel* infoLabel = new QLabel(combined);
                        infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
                        infoLabel->setWordWrap(true); // important for multi-line
                        gridLayout->addWidget(infoLabel, row, 0, 1, -1);
                        row++;

                    } else {
                        showQueryError(this, query, Q_FUNC_INFO);
                    }

                } // query destroyed here
            }
        }

        db.close(); // close connection
    } // db destroyed here

    // Now safe to remove the connection
    QSqlDatabase::removeDatabase("fetchviews");




    // Finally add a vertical spacer to push everything up
    QSpacerItem *verticalSpacer = new QSpacerItem(
        20, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    gridLayout->addItem(verticalSpacer, row+1, 0, 1, -1);



setupAlignedTimer();
updateFields();

    /*
     * Setup any autoclose singlshot timer
     */
    int autoClose = settings.getAutoCloseSeconds();
    if (autoClose > 0) {
        autoCloseTimer->stop();
        autoCloseTimer->start(autoClose * 1000);
    }

}

bool MainWindow::killGpgAgent()
{
    QProcess process;
    process.start("gpgconf", QStringList() << "--kill" << "gpg-agent");

    if (!process.waitForStarted(1500)) {
        qWarning().noquote() << Q_FUNC_INFO << "Failed to start gpgconf process";
        return false;
    }

    if (!process.waitForFinished(2500)) {
        qWarning().noquote() << Q_FUNC_INFO  << "gpgconf process did not finish";
        return false;
    }

    QByteArray output = process.readAllStandardOutput();
    QByteArray errors = process.readAllStandardError();

    if (!output.isEmpty())
        qDebug().noquote() << Q_FUNC_INFO  << "gpgconf output:" << output;
    if (!errors.isEmpty())
        qDebug().noquote() << Q_FUNC_INFO  << "gpgconf errors:" << errors;

    return (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
}

void MainWindow::setupAlignedTimer()
{
    // Calculate ms until next boundary (00 or 30)
    QTime now = QTime::currentTime();
    int msec = now.msec();
    int sec = now.second();

    int nextBoundarySec = (sec < 30) ? 30 : 60;
    int msUntilBoundary = ((nextBoundarySec - sec) * 1000) - msec;

    // One-shot timer to align with the next boundary
    QTimer::singleShot(msUntilBoundary, this, [this]() {
        // First tick
        updateFields();

        if (!alignedTimer)
            {
        alignedTimer = new QTimer(this);
        alignedTimer->setTimerType(Qt::PreciseTimer);

        connect(alignedTimer, &QTimer::timeout, this, [this]() {
            updateFields();

            // Recalculate alignment each tick
            QTime now = QTime::currentTime();
            int msec = now.msec();
            int sec = now.second();
            int nextBoundarySec = (sec < 30) ? 30 : 60;
            int msUntilBoundary = ((nextBoundarySec - sec) * 1000) - msec;

            alignedTimer->start(msUntilBoundary);
        });

        // Kick off the first 30‑second cycle
        alignedTimer->start(30 * 1000);
        }
    });

    if (!countdownTimer)
    {
        countdownTimer = new QTimer(this);
        countdownTimer->setTimerType(Qt::PreciseTimer);
        connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
        countdownTimer->start(1000);
    }
}


void MainWindow::updateCountdown()
{
    QTime now = QTime::currentTime();
    int sec = now.second();

    int remaining = (sec < 30) ? (30 - sec) : (60 - sec);
    countdownLabel->setText(QString("%1").arg(remaining, 2, 10, QChar('0')));
    countdownProgress->setValue(remaining);

    countdownLabel->setVisible(true);
    countdownProgress->setVisible(true);

    if (remaining > 15) {
        countdownProgress->setProperty("toptState", "normal");
    } else if (remaining > 5) {
        countdownProgress->setProperty("toptState", "warning");
    } else {
        countdownProgress->setProperty("toptState", "critical");
    }

    countdownProgress->style()->unpolish(countdownProgress);
    countdownProgress->style()->polish(countdownProgress);
    countdownProgress->update();


    updateFields();
}


void MainWindow::updateFields()
{
    // Store the result in a local variable first
    const QList<QLineEdit*> edits = ui->scrollArea->widget()->findChildren<QLineEdit*>();

    for (QLineEdit *edit : edits) {
        if (edit->objectName().startsWith("otpEdit")) {
            // Decode once and cache it
            if (!edit->placeholderText().isEmpty() && !edit->property("secret").isValid()) {
                QByteArray secret = base32Decode(edit->placeholderText());
                edit->setProperty("secret", secret);
            }

            // Use cached secret if available
            if (edit->property("secret").isValid()) {
                QByteArray secret = edit->property("secret").toByteArray();
                QString newValue = generateTOTP(secret, edit->property("otpLength").toInt());

                if (edit->text() != newValue) {
                    edit->setText(formatOtp(newValue));
                }
            } else {
                edit->setText("-");
            }
        }
    }
}

// Helper: convert counter to 8-byte big-endian
QByteArray MainWindow::intToBytes(quint64 counter)
{
    QByteArray result(8, 0);
    for (int i = 7; i >= 0; --i) {
        result[i] = static_cast<char>(counter & 0xFF);
        counter >>= 8;
    }
    return result;
}

// HMAC-SHA1 implementation using QCryptographicHash
QByteArray MainWindow::hmacSha1(const QByteArray &key, const QByteArray &message)
{
    const int blockSize = 64;
    QByteArray k = key;
    if (k.size() > blockSize)
        k = QCryptographicHash::hash(k, QCryptographicHash::Sha1);
    if (k.size() < blockSize)
        k.append(QByteArray(blockSize - k.size(), 0));

    QByteArray o_key_pad(blockSize, 0x5c);
    QByteArray i_key_pad(blockSize, 0x36);
    for (int i = 0; i < blockSize; ++i) {
        o_key_pad[i] = o_key_pad[i] ^ k[i];
        i_key_pad[i] = i_key_pad[i] ^ k[i];
    }

    QByteArray inner = QCryptographicHash::hash(i_key_pad + message, QCryptographicHash::Sha1);
    QByteArray hmac  = QCryptographicHash::hash(o_key_pad + inner, QCryptographicHash::Sha1);
    return hmac;
}

// Generate TOTP
QString MainWindow::generateTOTP(const QByteArray &secret, int digits, int step)
{
    quint64 counter = QDateTime::currentSecsSinceEpoch() / step;
    QByteArray msg = intToBytes(counter);
    QByteArray hash = hmacSha1(secret, msg);

    int offset = hash[hash.size() - 1] & 0x0F;
    quint32 binary =
        ((hash[offset] & 0x7f) << 24) |
        ((hash[offset + 1] & 0xff) << 16) |
        ((hash[offset + 2] & 0xff) << 8) |
        (hash[offset + 3] & 0xff);

    quint32 otp = binary % static_cast<quint32>(pow(10, digits));
    return QString("%1").arg(otp, digits, 10, QChar('0'));
}

QByteArray MainWindow::base32Decode(const QString &base32) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QByteArray result;
    int buffer = 0;
    int bitsLeft = 0;

    // Compute toUpper() once and store in a local variable
    const QString upper = base32.toUpper();

    for (QChar c : upper) {
        if (c == '=') {
            break;
        }
        const char *p = strchr(alphabet, c.toLatin1());
        if (!p) {
            continue; // skip invalid chars
        }

        int val = p - alphabet;
        buffer = (buffer << 5) | val;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            result.append(char((buffer >> bitsLeft) & 0xFF));
        }
    }
    return result;
}

void MainWindow::showPasswordsContextMenu(const QPoint &pos)
{
    QMenu menu;
    const QPoint globalPos = ui->treeWidget_2->viewport()->mapToGlobal(pos);

    const auto selectedItems    = ui->treeWidget_2->selectedItems();
    const auto selectedCategory = ui->treeWidget->selectedItems();

    // 1. Nothing selected at all → no menu
    if (selectedItems.isEmpty() && selectedCategory.isEmpty()) {
        return;
    }

    // 2. Category selected, but no password item selected
    if (selectedItems.isEmpty()) {
        qDebug() << "Hello"; // Placeholder for category-only actions
        menu.addAction(ui->actionNew_Password);
        menu.exec(globalPos);
        return;
    }

    menu.addAction(ui->actionNew_Password);
    menu.addSeparator();

    // 3. Password item selected
    QTreeWidgetItem *item = selectedItems.first();
    const int selectedId = item->data(0, Qt::UserRole).toInt();

    // 3a. Open / Close Password action
    if (openedCredentialID == selectedId) {
        auto *actionClose = new QAction(tr("Close Password"), &menu);
        actionClose->setIcon(QIcon(":/menus/glyphs/lock_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        actionClose->setShortcut(Qt::Key_Escape);
        actionClose->setStatusTip(tr("Hide the password and clear its decrypted details"));

        connect(actionClose, &QAction::hovered, [this, actionClose]() {
            statusBar()->showMessage(actionClose->statusTip());
        });
        connect(&menu, &QMenu::aboutToHide, [this]() {
            statusBar()->clearMessage();
        });
        connect(actionClose, &QAction::triggered, this, &MainWindow::clearScrollArea);

        menu.addAction(actionClose);
        menu.setDefaultAction(actionClose);
    } else {
        menu.addAction(ui->actionOpen_Password);
        menu.setDefaultAction(ui->actionOpen_Password);
    }

    // 3b. Core actions
    menu.addAction(ui->actionEdit_Password);
    menu.addAction(ui->actionAdd_Search);
    menu.addSeparator();
    menu.addAction(ui->actionDelete_Password);
    menu.addAction(ui->actionExport_Password);
    menu.addSeparator();

    // 4. Bookmark state
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        bool isBookmarked = false;

        if (db.open()) {
            QSqlQuery query(db);
            query.setForwardOnly(true);
            query.prepare(QStringLiteral(
                "SELECT 1 FROM favourite "
                "WHERE username = :username AND application_id = :id"));
            query.bindValue(":username", this->userName);
            query.bindValue(":id", selectedId);

            isBookmarked = (query.exec() && query.next());
        } else {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            return;
        }

        ui->actionBookmark->setCheckable(true);
        ui->actionBookmark->blockSignals(true);
        ui->actionBookmark->setChecked(isBookmarked);
        ui->actionBookmark->blockSignals(false);
    }
    QSqlDatabase::removeDatabase(connName);

    menu.addAction(ui->actionBookmark);
    menu.addSeparator();

    // 5. Audit log
    menu.addAction(ui->actionAudit_Log);

    // 6. Show menu
    menu.exec(globalPos);
}



QTreeWidgetItem* MainWindow::makeItemFromApplication(QSqlQuery& query) {
    int id       = query.value(0).toInt();
    //int parentId = query.value(1).toInt();
    QString name = DataObfuscator::deobfuscate(query.value(2).toString(),qApp->property("appKey").toByteArray());
    auto* item = new QTreeWidgetItem();
    item->setText(0, name);
    item->setData(0, Qt::UserRole, id);
    item->setIcon(0,QPixmap(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    qDebug().noquote() << id << name;
    return item;
}

void MainWindow::keyList()
{
    const QString dbFile = qApp->property("dbFile").toString();
    if (Q_UNLIKELY(dbFile.isEmpty())) {
        QMessageBox::warning(this, ui->actionKey_List->text(), tr("No database open."));
        return;
    }

    // Create dialog
    QDialog *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(ui->actionKey_List->text());
    dlg->setWindowFlags(dlg->windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction label
    QLabel *instruction = new QLabel(
        "Keys are linked only by their Key ID or fingerprint. No public or private keys are ever\nstored in this database.\n\nCurrently Linked Keys:",
        dlg
    );

    instruction->setWordWrap(false);
    instruction->setAlignment(Qt::AlignLeft);
    layout->addWidget(instruction);

    // Table widget
    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Key"));
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);

    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(false);
    table->horizontalHeader()->setFont(headerFont);

    // Populate from DB
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetchkeys");
        db.setDatabaseName(dbFile);
        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            return;
        }

        QSqlQuery query(db);
        if (query.exec("SELECT id, label, key FROM keys")) {
            int row = 0;
            while (query.next()) {
                table->insertRow(row);
                table->setItem(row, 0, new QTableWidgetItem(
                    DataObfuscator::deobfuscate(query.value(1).toString(), this->appKey)
                ));
                table->setItem(row, 1, new QTableWidgetItem(
                    DataObfuscator::deobfuscate(query.value(2).toString(), this->appKey)
                ));
                table->item(row, 1)->setData(Qt::UserRole, query.value(0).toInt());
                row++;
            }
        } else {
            showQueryError(this, query, Q_FUNC_INFO);
        }
    }
    QSqlDatabase::removeDatabase("fetchkeys");

    layout->addWidget(table);
    table->setCurrentItem(nullptr);
    table->clearSelection();

    //
    // BUTTONS
    //
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    // NEW KEY BUTTON
    QPushButton *newKeyBtn = new QPushButton("&New Key", dlg);

    // SHOW TOOLBUTTON
    QToolButton *showBtn = new QToolButton(dlg);
    showBtn->setText("Show");
    showBtn->setPopupMode(QToolButton::MenuButtonPopup);

    QMenu *showMenu = new QMenu(showBtn);
    QAction *showPublicAct = showMenu->addAction("Public Keys");
    QAction *showSecretAct = showMenu->addAction("Secret Keys");
    showPublicAct->setIcon(QIcon(":/menus/glyphs/key_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    showSecretAct->setIcon(QIcon(":/menus/glyphs/key_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    showBtn->setMenu(showMenu);

    // EXISTING BUTTONS
    QPushButton *importBtn = new QPushButton("&Link Key", dlg);
    QPushButton *deleteBtn = new QPushButton("&Unlink", dlg);
    QPushButton *helpBtn   = new QPushButton("&Help", dlg);
    QPushButton *closeBtn  = new QPushButton("&Close", dlg);

    importBtn->setDefault(true);

    buttonLayout->addWidget(newKeyBtn);
    buttonLayout->addWidget(showBtn);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addWidget(deleteBtn);
    buttonLayout->addWidget(helpBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);

    layout->addLayout(buttonLayout);

    connect(showBtn, &QPushButton::clicked, this, [=]() {
        showGpgKeyListDialog(GpgKeyType::Public, this->userName, dlg);
    });

    //
    // NEW KEY HANDLER
    //
    connect(newKeyBtn, &QPushButton::clicked, this, [=]() {
        QDialog inputDlg(dlg);
        inputDlg.setWindowTitle(tr("New GPG Key"));

        QVBoxLayout *vbox = new QVBoxLayout(&inputDlg);

        // Build a suggested name based on the current user
        QString suggestedName;
        if (!this->userName.isEmpty()) {
            suggestedName = this->userName + tr("'s passwords");
        } else {
            suggestedName = tr("My passwords");
        }

        // Instruction label
        QLabel *infoLabel = new QLabel(
            tr("Enter a name for the new key.\nThis name identifies the key in your GPG keyring.\n\n"
               "Examples: \"%1\", \"Work passwords\", \"Personal vault\".\n")
                .arg(suggestedName),
            &inputDlg
        );
        infoLabel->setWordWrap(false);
        vbox->addWidget(infoLabel);

        // Form layout
        QFormLayout *form = new QFormLayout();
        QLineEdit *nameEdit = new QLineEdit(&inputDlg);
        nameEdit->setPlaceholderText(suggestedName);
        form->addRow(tr("Name:"), nameEdit);
        vbox->addLayout(form);

        // Buttons
        QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            &inputDlg
        );
        vbox->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, &inputDlg, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &inputDlg, &QDialog::reject);

        if (inputDlg.exec() == QDialog::Accepted) {
            QString name = nameEdit->text().trimmed();
            if (name.isEmpty()) {
                // If user leaves it blank, fall back to the suggested name
                name = suggestedName;
            }

            // Asynchronous key creation — UI stays responsive
            createGpgEncryptionKeyAsync(name, dlg, [dlg](bool ok) {
                if (ok) {
                    QMessageBox::information(dlg, tr("Success"), tr("New GPG key created."));
                }
                // Errors are already shown inside the async function
            });
        }
    });




    //
    // SHOW PUBLIC / SECRET KEYS
    //
    connect(showPublicAct, &QAction::triggered, this, [=]() {
        showGpgKeyListDialog(GpgKeyType::Public, this->userName, dlg);
    });

    connect(showSecretAct, &QAction::triggered, this, [=]() {
        showGpgKeyListDialog(GpgKeyType::Secret, this->userName, dlg);
    });

    //
    // EXISTING IMPORT BUTTON
    //
    connect(importBtn, &QPushButton::clicked, this, [=]() {
        QDialog inputDlg(dlg);
        inputDlg.setWindowTitle(tr("Link Key"));

        QVBoxLayout *vbox = new QVBoxLayout(&inputDlg);

        // 🔹 Warning label at the top
        QLabel *warnLabel = new QLabel(
            tr("Ensure the private key is available in your GPG keyring or on a hardware token. "
               "Without it, passwords encrypted with this key cannot be decrypted."),
            &inputDlg
        );
        warnLabel->setWordWrap(true);
        warnLabel->setStyleSheet("color:#b22222;"); // optional: subtle warning color
        vbox->addWidget(warnLabel);

        // 🔹 Form layout
        QFormLayout *form = new QFormLayout();
        QLineEdit *keyEdit  = new QLineEdit(&inputDlg);
        QLineEdit *nameEdit = new QLineEdit(&inputDlg);

        keyEdit->setPlaceholderText(tr("Enter GPG Key ID or fingerprint"));
        nameEdit->setPlaceholderText(tr("Enter name / label"));

        form->addRow(tr("GPG Key:"), keyEdit);
        form->addRow(tr("Name:"), nameEdit);

        vbox->addLayout(form);

        // 🔹 Buttons
        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                 Qt::Horizontal, &inputDlg);
        vbox->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, &inputDlg, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &inputDlg, &QDialog::reject);

        inputDlg.setFixedSize((dlg->width() / 4)*3, inputDlg.sizeHint().height());

        if (inputDlg.exec() == QDialog::Accepted) {
            QString keyId = keyEdit->text().trimmed();
            QString name  = nameEdit->text().trimmed();

            if (keyId.isEmpty() || name.isEmpty()) {
                QMessageBox::warning(dlg, tr("Invalid Input"),
                                     tr("Both key ID and name are required."));
                return;
            }

            // Duplicate check
            for (int r = 0; r < table->rowCount(); ++r) {
                if (table->item(r, 1) && table->item(r, 1)->text() == keyId) {
                    QMessageBox::warning(dlg, tr("Duplicate Key"),
                                         tr("This key ID is already in the list."));
                    return;
                }
            }

            // Trust check
            if (!hasUltimateTrust(keyId)) {
                QMessageBox msgBox(dlg);
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setWindowTitle(tr("Key"));
                msgBox.setText(tr("The key does not have ultimate trust.\n"
                                  "Untrusted keys cannot be used to encrypt passwords."));
                msgBox.setStandardButtons(QMessageBox::Ok);
                QPushButton *helpButton = msgBox.addButton(QMessageBox::Help);

                connect(helpButton, &QPushButton::clicked, this, [this]() {
                    checkHelpReachable([this](bool reachable) {
                        if (reachable) {
                            const QUrl url(Passwords::HelpBaseUrl + QStringLiteral("keys"));
                            QDesktopServices::openUrl(url);
                        } else {
                            launchHelperProcess(QStringLiteral("keys"));
                        }
                    });
                });

                msgBox.exec();
                return;
            }

            // Insert into DB
            {
                QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insertkeys");
                db.setDatabaseName(qApp->property("dbFile").toString());
                if (db.open()) {
                    QSqlQuery insert(db);
                    insert.prepare("INSERT INTO keys (label, key) VALUES (:label, :key)");
                    insert.bindValue(":label", DataObfuscator::obfuscate(name, this->appKey));
                    insert.bindValue(":key", DataObfuscator::obfuscate(keyId, this->appKey));
                    if (!insert.exec()) {
                        showQueryError(this, insert, Q_FUNC_INFO);
                        return;
                    }
                } else {
                    showDbNotOpenError(this, db, Q_FUNC_INFO);
                }
            }
            QSqlDatabase::removeDatabase("insertkeys");

            // Update table dynamically
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(name));
            table->setItem(row, 1, new QTableWidgetItem(keyId));
        }
    });


    //
    // DELETE BUTTON
    //
    connect(deleteBtn, &QPushButton::clicked, this, [=]() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(dlg, "Delete Key", "Please select a key to delete.");
            return;
        }

        QString name  = table->item(row, 0)->text();
        QString keyId = table->item(row, 1)->text();

        if (QMessageBox::question(
                dlg,
                tr("Unlink Key"),
                QString(tr("Remove key '%1' (%2) from the list of available keys?\n\nThis will not affect existing passwords, and the key will remain in your GPG keyring."))
                    .arg(name, keyId))
            != QMessageBox::Yes)
        {
            return;
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "deletekeys");
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (db.open()) {

                if (!settings.verifyDeleteAllowed(db, this)) {
                    QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
                    return;
                }

                QSqlQuery remove(db);
                remove.prepare("DELETE FROM keys WHERE id = :id");
                remove.bindValue(":id", table->item(row, 1)->data(Qt::UserRole).toInt());
                if (!remove.exec()) {
                    showQueryError(this, remove, Q_FUNC_INFO);
                    return;
                }
            } else {
                showDbNotOpenError(this, db, Q_FUNC_INFO);
            }
        }
        QSqlDatabase::removeDatabase("deletekeys");

        table->removeRow(row);
    });

    table->resizeColumnsToContents();

    dlg->layout()->setSizeConstraint(QLayout::SetFixedSize);
    dlg->setFixedSize(dlg->sizeHint());

    // HELP BUTTON
    connect(helpBtn, &QPushButton::clicked, this, [this]() {
        checkHelpReachable([this](bool reachable) {
            if (reachable) {
                const QUrl url(Passwords::HelpBaseUrl + QStringLiteral("linking-keys"));
                QDesktopServices::openUrl(url);
            } else {
                launchHelperProcess(QStringLiteral("linking-keys"));
            }
        });
    });

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->setLayout(layout);
    dlg->resize(400, 300);
    dlg->exec();
}


void MainWindow::showAuditLog(QTreeWidgetItem *item)
{
    /*
     * Password Audit Log
     */

    // Create dialog
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionAudit_Log->text());

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction label
    QLabel *instruction = new QLabel(tr( "Audit Log:"),dlg);
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    // Table widget
    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(5); // audit_id, dt, user, host, action
    table->setHorizontalHeaderLabels({ tr("Audit ID"), tr("Date/Time"), tr("User"), tr("Host"), tr("Action") });
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);

    // Populate from DB
    int appId = item->data(0, Qt::UserRole).toInt();
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(qApp->property("dbFile").toString());
    if (!db.open()) {
        qCritical().noquote() << Q_FUNC_INFO << "No database open." << db.lastError().text();
        return;
    } else {
        QSqlQuery query(db);
        query.prepare(R"(
    SELECT audit_id,
           datetime(dt, 'unixepoch', 'localtime') AS dt_human,
           user,
           host,
           action
    FROM application_views_audit
    WHERE application_id = :application_id
)");

        query.bindValue(":application_id", appId);

        if (query.exec()) {
            int row = 0;
            while (query.next()) {
                table->insertRow(row);
                table->setItem(row, 0, new QTableWidgetItem(query.value("audit_id").toString()));
                table->setItem(row, 1, new QTableWidgetItem(query.value("dt_human").toString()));
                table->setItem(row, 2, new QTableWidgetItem(query.value("user").toString()));
                table->setItem(row, 3, new QTableWidgetItem(query.value("host").toString()));
                table->setItem(row, 4, new QTableWidgetItem(query.value("action").toString()));
                row++;
            }
        } else {
            showQueryError(this,query,Q_FUNC_INFO);
        }
    }

    layout->addWidget(table);
    table->resizeColumnsToContents();
    table->setCurrentItem(nullptr);
    table->clearSelection();

    // Buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *exportBtn = new QPushButton("&Export", dlg);
    QPushButton *deleteBtn = new QPushButton("&Delete", dlg);   // <‑‑‑‑‑ Added
    QPushButton *closeBtn = new QPushButton("&Close", dlg);

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(deleteBtn);                         // <‑‑‑‑‑ Added
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Export button handler
    connect(exportBtn, &QPushButton::clicked, this, [table, dlg]() {
        QString fileName = QFileDialog::getSaveFileName(dlg, "Export Audit Log",
                                                        QDir::homePath(),
                                                        "CSV Files (*.csv)");
        if (fileName.isEmpty()) return;

        if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            fileName += ".csv";
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(dlg, "Export Failed", "Could not open file for writing.");
            return;
        }

        QTextStream out(&file);

        // Write header
        QStringList headers;
        for (int col = 0; col < table->columnCount(); ++col) {
            headers << table->horizontalHeaderItem(col)->text();
        }
        out << headers.join(",") << "\n";

        // Write rows
        for (int row = 0; row < table->rowCount(); ++row) {
            QStringList fields;
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem *item = table->item(row, col);
                QString text = item ? item->text() : "";
                fields << "\"" + text.replace("\"", "\"\"") + "\"";
            }
            out << fields.join(",") << "\n";
        }

        file.close();
        QMessageBox::information(dlg, "Export Complete", "Audit log exported successfully.");
    });

    connect(deleteBtn, &QPushButton::clicked, this, [this, dlg, appId, connName]() {
        if (QMessageBox::warning(
                dlg,
                tr("Confirm Delete"),
                tr("This will permanently delete all audit log entries and view records "
                   "for this application.\n\nAre you sure you want to continue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        QSqlDatabase db = QSqlDatabase::database(connName);
        if (!db.isOpen()) {
            showDbNotOpenError(this,db,Q_FUNC_INFO);
            return;
        }

        if (!settings.verifyDeleteAllowed(db, this)) {
            QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
            return;
        }

        QSqlQuery query(db);

        if (!db.transaction()) {
            showTransactionError(this,db,Q_FUNC_INFO);
            return;
        }

        query.prepare("DELETE FROM application_views_audit WHERE application_id = :id");
        query.bindValue(":id", appId);
        if (!query.exec()) {
            db.rollback();
            showQueryError(this, query, Q_FUNC_INFO);
            return;
        }

        query.prepare("DELETE FROM application_views WHERE application_id = :id");
        query.bindValue(":id", appId);
        if (!query.exec()) {
            db.rollback();
            showQueryError(this, query, Q_FUNC_INFO);
            return;
        }

        if (!db.commit()) {
            db.rollback();
            showTransactionError(this,db,Q_FUNC_INFO);
            return;
        }

        db.close();
        insertAuditRow(appId,this->userName,QSysInfo::machineHostName(),"AUDIT LOG CLEARED");

        QMessageBox::information(dlg, tr("Deleted"), tr("The audit log has been successfully cleared."));
        dlg->accept();
    });

    dlg->setLayout(layout);
    dlg->resize(700, 400);
    dlg->exec();
}

void MainWindow::encryptMessage()
{
    /*
     * Encrypt Message Dialog (GPG symmetric, ASCII-armored)
     * Now uses secure stdin passphrase (no command-line exposure).
     */

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionEncrypt_message->text());
    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *instruction = new QLabel(
        "Enter the message and a password to encrypt with GPG (symmetric, ASCII-armored).\n"
        "The encrypted text will appear below.",
        dlg
    );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    QTextEdit *plainTextEdit = new PlainTextEdit(dlg);
    plainTextEdit->setObjectName("plainTextInput");
    plainTextEdit->setPlaceholderText("Enter text to encrypt...");
    plainTextEdit->setAcceptRichText(false);
    layout->addWidget(plainTextEdit);

    // Password row
    QHBoxLayout *passLayout = new QHBoxLayout;

    QLabel *passLabel1 = new QLabel("Password:", dlg);
    QLineEdit *passEdit1 = new QLineEdit(dlg);
    passEdit1->setEchoMode(settings.getEchoMode());
    passEdit1->setPlaceholderText("Enter password");

    QLabel *passLabel2 = new QLabel("Confirm:", dlg);
    QLineEdit *passEdit2 = new QLineEdit(dlg);
    passEdit2->setEchoMode(settings.getEchoMode());
    passEdit2->setPlaceholderText("Re-enter password");

    QToolButton *generateBtn = new QToolButton(dlg);
    generateBtn->setText("Generate");
    generateBtn->setPopupMode(QToolButton::MenuButtonPopup);

    QMenu *menu = new QMenu(generateBtn);
    menu->addAction(ui->actionGenerate_Password);
    menu->addAction(ui->actionRandom_Noise);
    generateBtn->setMenu(menu);

    passLayout->addWidget(passLabel1);
    passLayout->addWidget(passEdit1);
    passLayout->addSpacing(20);
    passLayout->addWidget(passLabel2);
    passLayout->addWidget(passEdit2);
    passLayout->addSpacing(20);
    passLayout->addWidget(generateBtn);

    layout->addLayout(passLayout);

    // Encrypted output
    QTextEdit *encryptedTextEdit = new QTextEdit(dlg);
    encryptedTextEdit->setReadOnly(true);
    encryptedTextEdit->setPlaceholderText("Encrypted text will appear here...");
    layout->addWidget(encryptedTextEdit);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *encryptBtn = new QPushButton("&Encrypt", dlg);
    QPushButton *copyBtn    = new QPushButton("Copy", dlg);
    QPushButton *saveBtn    = new QPushButton("&Save", dlg);
    QPushButton *closeBtn   = new QPushButton("&Close", dlg);
    saveBtn->setEnabled(false);
    encryptBtn->setDefault(true);
    buttonLayout->addWidget(encryptBtn);
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Password generator actions
    connect(generateBtn, &QToolButton::clicked, this,
            [this]() {
                PasswordDialog::showPasswordGenerator(
                    this,
                    ui->actionGenerate_Password->text(),
                    {}
                );
            });

    // --- SECURE ENCRYPTION ACTION ---
    connect(encryptBtn, &QPushButton::clicked, this,
            [plainTextEdit, passEdit1, passEdit2, encryptedTextEdit]() {

        const QString inputText = plainTextEdit->toPlainText();
        QString pass1 = passEdit1->text();
        QString pass2 = passEdit2->text();

        if (inputText.isEmpty()) {
            QMessageBox::warning(nullptr, "No Input", "Please enter some text to encrypt.");
            return;
        }
        if (pass1.isEmpty() || pass2.isEmpty()) {
            QMessageBox::warning(nullptr, "No Password", "Please enter and confirm a password.");
            return;
        }
        if (pass1 != pass2) {
            QMessageBox::warning(nullptr, "Mismatch", "Passwords do not match. Please re-enter.");
            return;
        }

        if (!isStrong(pass1)) {
            if (!warnAndContinue())
                return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();

        QProcess process;
        QStringList args;
        args << "--batch"
             << "--yes"
             << "--pinentry-mode" << "loopback"
             << "--symmetric"
             << "-a"
             << "--passphrase-fd" << "0";   // SECURE: passphrase via stdin

        process.start("gpg", args);
        if (!process.waitForStarted()) {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(nullptr, "Error", "Failed to start gpg process.");
            return;
        }

        // --- Write password securely ---
        QByteArray passBytes = pass1.toUtf8();
        passBytes.append('\n');
        process.write(passBytes);

        // Wipe password from memory
        std::fill(passBytes.begin(), passBytes.end(), 0);
        std::fill(pass1.begin(), pass1.end(), QChar(0));
        std::fill(pass2.begin(), pass2.end(), QChar(0));

        // --- Write plaintext ---
        process.write(inputText.toUtf8());
        process.closeWriteChannel();

        if (!process.waitForFinished()) {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(nullptr, "Error", "gpg process did not finish.");
            return;
        }

        const QString encryptedOutput = process.readAllStandardOutput();
        const QString errorOutput     = process.readAllStandardError();

        if (!encryptedOutput.isEmpty()) {
            encryptedTextEdit->setPlainText(encryptedOutput);
        } else {
            encryptedTextEdit->setPlainText("Encryption failed:\n" + errorOutput);
        }

        QApplication::restoreOverrideCursor();
    });

    // Copy
    connect(copyBtn, &QPushButton::clicked, this,
            [this, encryptedTextEdit]() {
                const QString text = encryptedTextEdit->toPlainText();
                if (!text.isEmpty()) {
                    QGuiApplication::clipboard()->setText(text);
                    statusBar()->showMessage("Encrypted text copied to clipboard.", 3000);
                } else {
                    statusBar()->showMessage("There is no encrypted text to copy.", 3000);
                }
            });

    connect(plainTextEdit, &QTextEdit::textChanged, this,
            [plainTextEdit, encryptedTextEdit]() {
                if (plainTextEdit->toPlainText().isEmpty())
                    encryptedTextEdit->clear();
            });

    connect(encryptedTextEdit, &QTextEdit::textChanged, this,
            [encryptedTextEdit, saveBtn]() {
                saveBtn->setEnabled(!encryptedTextEdit->toPlainText().isEmpty());
            });

    // Save
    connect(saveBtn, &QPushButton::clicked, this,
            [encryptedTextEdit, dlg]() {
                const QString text = encryptedTextEdit->toPlainText();
                if (text.isEmpty()) return;

                QString fileName = QFileDialog::getSaveFileName(
                    dlg,
                    QObject::tr("Save Encrypted Text"),
                    QString(),
                    QObject::tr("ASCII-armored Files (*.asc);;Text Files (*.txt);;All Files (*)")
                );

                if (fileName.isEmpty())
                    return;

                // --- Ensure .asc extension if none provided ---
                QFileInfo fi(fileName);
                if (fi.suffix().isEmpty()) {
                    fileName += ".asc";
                }

                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << text;
                    file.close();
                    QMessageBox::information(dlg, "Saved",
                                             "Encrypted text saved successfully.");
                } else {
                    QMessageBox::critical(dlg, "Error",
                                          "Could not save file.");
                }
            });

    dlg->setLayout(layout);
    dlg->resize(700, 550);
    dlg->exec();
}


void MainWindow::decryptMessage()
{
    /*
     * Decrypt Message Dialog (GPG symmetric, ASCII-armored)
     * Now uses secure stdin passphrase (no command-line exposure).
     */

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionDecrypt_message->text());
    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *instruction = new QLabel(
        "Paste the encrypted message below and enter the password used to encrypt it.\n"
        "The decrypted plaintext will appear underneath.",
        dlg
    );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    QTextEdit *encryptedInputEdit = new QTextEdit(dlg);
    encryptedInputEdit->setPlaceholderText("Paste encrypted text here...");
    layout->addWidget(encryptedInputEdit);

    QHBoxLayout *passLayout = new QHBoxLayout;
    QLabel *passLabel = new QLabel("Password:", dlg);
    QLineEdit *passEdit = new QLineEdit(dlg);
    passEdit->setEchoMode(settings.getEchoMode());
    passEdit->setPlaceholderText("Enter password");

    passLayout->addWidget(passLabel);
    passLayout->addWidget(passEdit);
    layout->addLayout(passLayout);

    QTextEdit *decryptedTextEdit = new QTextEdit(dlg);
    decryptedTextEdit->setReadOnly(true);
    decryptedTextEdit->setPlaceholderText("Decrypted text will appear here...");
    layout->addWidget(decryptedTextEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    QPushButton *copyBtn    = new QPushButton("Copy", dlg);
    QPushButton *decryptBtn = new QPushButton("&Decrypt", dlg);
    QPushButton *closeBtn   = new QPushButton("&Close", dlg);

    copyBtn->setEnabled(false);
    decryptBtn->setDefault(true);

    buttonLayout->addWidget(decryptBtn);
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // --- SECURE DECRYPT ACTION ---
    connect(decryptBtn, &QPushButton::clicked, this,
            [encryptedInputEdit, passEdit, decryptedTextEdit]() {

        const QString encryptedText = encryptedInputEdit->toPlainText();
        QString pass = passEdit->text();

        if (encryptedText.isEmpty()) {
            QMessageBox::warning(nullptr, "No Input", "Please paste some encrypted text.");
            return;
        }
        if (pass.isEmpty()) {
            QMessageBox::warning(nullptr, "No Password", "Please enter a password.");
            return;
        }

        QProcess process;
        QStringList args;
        args << "--batch"
             << "--yes"
             << "--pinentry-mode" << "loopback"
             << "--decrypt"
             << "--passphrase-fd" << "0";   // SECURE: passphrase via stdin

        process.start("gpg", args);
        if (!process.waitForStarted()) {
            QMessageBox::critical(nullptr, "Error", "Failed to start gpg process.");
            return;
        }

        // --- Write password securely ---
        QByteArray passBytes = pass.toUtf8();
        passBytes.append('\n');
        process.write(passBytes);

        // Wipe password from memory
        std::fill(passBytes.begin(), passBytes.end(), 0);
        std::fill(pass.begin(), pass.end(), QChar(0));

        // --- Write encrypted text ---
        process.write(encryptedText.toUtf8());
        process.closeWriteChannel();

        if (!process.waitForFinished()) {
            QMessageBox::critical(nullptr, "Error", "gpg process did not finish.");
            return;
        }

        const QString decryptedOutput = process.readAllStandardOutput();
        const QString errorOutput     = process.readAllStandardError();

        if (!decryptedOutput.isEmpty()) {
            decryptedTextEdit->setPlainText(decryptedOutput);
        } else {
            decryptedTextEdit->setPlainText("Decryption failed:\n" + errorOutput);
        }
    });

    connect(decryptedTextEdit, &QTextEdit::textChanged, this,
            [decryptedTextEdit, copyBtn]() {
                copyBtn->setEnabled(!decryptedTextEdit->toPlainText().isEmpty());
            });

    connect(encryptedInputEdit, &QTextEdit::textChanged, this,
            [encryptedInputEdit, decryptedTextEdit]() {
                if (encryptedInputEdit->toPlainText().isEmpty())
                    decryptedTextEdit->clear();
            });

    connect(copyBtn, &QPushButton::clicked, this,
            [decryptedTextEdit, dlg]() {
                const QString text = decryptedTextEdit->toPlainText();
                if (!text.isEmpty()) {
                    QGuiApplication::clipboard()->setText(text);
                    QMessageBox::information(dlg, "Copied", "Decrypted text copied to clipboard.");
                }
            });

    dlg->setLayout(layout);
    dlg->resize(700, 500);
    dlg->exec();
}

void MainWindow::encryptFile()
{
    /*
     * Encrypt File Dialog (GPG symmetric, binary or ascii output)
     * Uses --pinentry-mode loopback and a passphrase provided via stdin.
     * Single dialog for file selection + password (with confirmation).
     */

    EncryptFileDialog dlg(this, ui->actionEncrypt_File->text());
    if (dlg.exec() != QDialog::Accepted)
        return; // user cancelled

    QString inputFile  = dlg.inputFile();
    QString outputFile = dlg.outputFile();
    QString password   = dlg.password();
    bool asciiArmor    = dlg.asciiArmor();

    // Warn if output already exists
    if (QFileInfo::exists(outputFile)) {
        QMessageBox::StandardButton reply =
            QMessageBox::question(
                this,
                tr("Overwrite File?"),
                tr("The file \"%1\" already exists.\n"
                   "Do you want to overwrite it?")
                    .arg(outputFile),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );

        if (reply != QMessageBox::Yes)
            return;
    }

    ui->statusbar->showMessage(
        tr("Encrypting %1 in the background...").arg(inputFile)
    );

    // Create process
    QProcess *process = new QProcess(this);

    QStringList args;
    args << "--batch"
         << "--yes"
         << "--pinentry-mode" << "loopback"
         << "--symmetric"
         << "--passphrase-fd" << "0";   // SECURE: passphrase via stdin

    if (asciiArmor)
        args << "--armor";

    //args << "-o" << outputFile
    args << QFileInfo(inputFile).fileName();

    process->setWorkingDirectory(QFileInfo(inputFile).absolutePath());

    // Handle completion (success or failure)
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process, outputFile](int exitCode, QProcess::ExitStatus status) {
                QString err = process->readAllStandardError();
                if (status == QProcess::NormalExit && exitCode == 0) {
                    QMessageBox::information(this, ui->actionEncrypt_File->text(),
                                             tr("File encrypted successfully:\n%1")
                                                 .arg(outputFile));
                } else {
                    QMessageBox::critical(this, ui->actionEncrypt_File->text(),
                                          tr("Encryption failed:\n%1").arg(err));
                }
                process->deleteLater();
            });

    // Handle start errors
    connect(process, &QProcess::errorOccurred,
            this,
            [this, process](QProcess::ProcessError error) {
                QMessageBox::critical(
                    this,
                    tr("Error"),
                    tr("Failed to start gpg process (process error code %1).")
                        .arg(QString::number(error))
                );
                process->deleteLater();
            });

    process->start("gpg", args);
    if (process->state() == QProcess::NotRunning) {
        QMessageBox::critical(this, ui->actionEncrypt_File->text(),
                              tr("Failed to start gpg process."));
        process->deleteLater();
        return;
    }

    // --- Send password securely via stdin ---
    QByteArray passBytes = password.toUtf8();
    passBytes.append('\n');  // GPG expects newline for passphrase input

    process->write(passBytes);
    process->closeWriteChannel();

    // Wipe password from memory
    std::fill(passBytes.begin(), passBytes.end(), 0);
    std::fill(password.begin(), password.end(), QChar(0));
}




void MainWindow::decryptFile()
{
    const QString inputFile = QFileDialog::getOpenFileName(
        this,
        ui->actionDecrypt_File->text(),
        QString(),
        tr("Encrypted Files (*.gpg *.asc);;All Files (*)")
    );
    if (inputFile.isEmpty())
        return;

    // --- Password dialog ---
    QDialog passDlg(this);
    passDlg.setWindowTitle(ui->actionDecrypt_File->text());

    QVBoxLayout layout(&passDlg);
    QLabel label(tr("Enter the password to decrypt the file:"), &passDlg);
    QLineEdit passEdit(&passDlg);
    passEdit.setEchoMode(settings.getEchoMode());

    QPushButton okBtn("&OK", &passDlg);
    QPushButton cancelBtn("&Cancel", &passDlg);

    QHBoxLayout btnLayout;
    btnLayout.addStretch();
    btnLayout.addWidget(&okBtn);
    btnLayout.addWidget(&cancelBtn);

    layout.addWidget(&label);
    layout.addWidget(&passEdit);
    layout.addLayout(&btnLayout);

    connect(&okBtn, &QPushButton::clicked, &passDlg, &QDialog::accept);
    connect(&cancelBtn, &QPushButton::clicked, &passDlg, &QDialog::reject);

    if (passDlg.exec() != QDialog::Accepted)
        return;

    QString password = passEdit.text();
    if (password.isEmpty()) {
        QMessageBox::warning(this,
                             ui->actionDecrypt_File->text(),
                             tr("You must enter a password."));
        return;
    }

    ui->statusbar->showMessage(
        tr("Decrypting %1 in the background...").arg(inputFile), 0);

    // --- GPG process ---
    auto *process = new QProcess(this);

    QStringList args;
    args << "--batch"
         << "--yes"
         << "--pinentry-mode" << "loopback"
         << "--decrypt"
         << "--passphrase-fd" << "0"
         << "--use-embedded-filename"
         << inputFile;

    const QFileInfo fi(inputFile);
    process->setWorkingDirectory(fi.absolutePath());

    // Buffers (RAII)
    auto stdoutBuffer = std::make_shared<QByteArray>();
    auto stderrBuffer = std::make_shared<QByteArray>();

    connect(process, &QProcess::readyReadStandardOutput, this,
            [process, stdoutBuffer]() {
                stdoutBuffer->append(process->readAllStandardOutput());
            });

    connect(process, &QProcess::readyReadStandardError, this,
            [process, stderrBuffer, this]() {
                const QByteArray err = process->readAllStandardError();
                if (!err.isEmpty()) {
                    stderrBuffer->append(err);
                    ui->statusbar->showMessage(QString::fromUtf8(err));
                }
            });

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process, inputFile, stdoutBuffer, stderrBuffer]
            (int exitCode, QProcess::ExitStatus status)
    {
        const QString stderrText = QString::fromUtf8(*stderrBuffer);
        const bool success = (status == QProcess::NormalExit && exitCode == 0);

        // Detect embedded filename
        QString embeddedOutputPath;
        {
            static const QRegularExpression re("writing to '([^']+)'");
            const QRegularExpressionMatch m = re.match(stderrText);
            if (m.hasMatch())
                embeddedOutputPath = m.captured(1);
        }

        if (success) {
            if (!embeddedOutputPath.isEmpty()) {
                ui->statusbar->showMessage("Decryption complete", 5000);
                QMessageBox::information(
                    this,
                    ui->actionDecrypt_File->text(),
                    tr("File decrypted successfully:\n") + embeddedOutputPath
                );
            }
            else if (!stdoutBuffer->isEmpty()) {
                const QFileInfo fi(inputFile);
                const QString suggestedName =
                    fi.absolutePath() + "/" +
                    fi.completeBaseName() + ".decrypted";

                const QString savePath = QFileDialog::getSaveFileName(
                    this,
                    tr("Save Decrypted Output"),
                    suggestedName,
                    tr("All Files (*)")
                );

                if (!savePath.isEmpty()) {
                    QFile out(savePath);
                    if (out.open(QIODevice::WriteOnly)) {
                        out.write(*stdoutBuffer);
                        out.close();

                        ui->statusbar->showMessage("Decryption complete", 5000);
                        QMessageBox::information(
                            this,
                            ui->actionDecrypt_File->text(),
                            tr("Decrypted data saved to:\n") + savePath
                        );
                    } else {
                        QMessageBox::critical(
                            this,
                            ui->actionDecrypt_File->text(),
                            tr("Could not write output file:\n") + savePath
                        );
                    }
                } else {
                    QMessageBox::warning(
                        this,
                        ui->actionDecrypt_File->text(),
                        tr("Decryption succeeded, but no file was saved.")
                    );
                }
            }
            else {
                QMessageBox::information(
                    this,
                    ui->actionDecrypt_File->text(),
                    tr("Decryption succeeded, check your source directory.")
                );
            }
        }
        else {
            ui->statusbar->showMessage(tr("Decryption failed"), 5000);
            QMessageBox::critical(
                this,
                ui->actionDecrypt_File->text(),
                tr("Decryption failed:\n") + stderrText
            );
        }

        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
                ui->statusbar->showMessage(tr("Failed to start gpg process"), 5000);
                QMessageBox::critical(
                    this,
                    ui->actionDecrypt_File->text(),
                    tr("Failed to start gpg process.")
                );
            });

    // --- Start process ---
    process->start("gpg", args);

    // --- Send password securely via stdin ---
    QByteArray passBytes = password.toUtf8();
    passBytes.append('\n');  // GPG expects newline

    process->write(passBytes);
    process->closeWriteChannel();

    // Wipe password from memory
    std::fill(passBytes.begin(), passBytes.end(), 0);
    std::fill(password.begin(), password.end(), QChar(0));
}


bool MainWindow::openDatabase(const QString &fileName)
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(fileName);

        if (!db.open()) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to open database: %1").arg(db.lastError().text()));
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        QSqlQuery(db).exec("PRAGMA foreign_keys = ON");

        QSqlQuery query(db);
        if (!query.exec("SELECT key, value FROM app_info WHERE key IN ('app_signature','schema_version')")) {
            showQueryError(this,query,Q_FUNC_INFO);
            db.close();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        QString signature, schemaVersion;
        while (query.next()) {
            const QString key = query.value(0).toString();
            const QString val = query.value(1).toString();
            if (key == "app_signature") signature = val;
            else if (key == "schema_version") schemaVersion = val;
        }

        if (signature == MainWindow::APP_SIGNATURE &&
            schemaVersion == MainWindow::SCHEMA_VERSION) {
            qApp->setProperty("dbFile", fileName);
            init();
        } else {
            qApp->setProperty("dbFile", QString());
            QMessageBox::critical(this, tr("Error"),
                                  tr("This database is not a valid passwords database.\n"
                                     "It may also belong to a different version of this program (invalid signature or schema version)."));
            db.close();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    // Backup only if database was valid
    if (settings.getBackupDatabase()) {
        QFileInfo fi(fileName);
        QString backupDirPath = fi.absolutePath() + "/backups";
        QDir backupDir(backupDirPath);
        if (!backupDir.exists()) {
            backupDir.mkpath(".");
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupPath = backupDirPath + "/" + fi.completeBaseName() + "_" + timestamp;

        if (!QFile::copy(fileName, backupPath)) {
            qCritical().noquote() << Q_FUNC_INFO << "Failed to create backup:" << QDir::toNativeSeparators(backupPath) + ":" << QFile(fileName).errorString();
        } else {
            qInfo().noquote() << "Backup created as:" << backupPath;
        }
    }

    return true;
}

void MainWindow::init()
{
    /*
     * openDatabase initilizaton tasks.
     */
    this->blockSignals(true);
    clearScrollArea();
    ui->treeWidget->clear();
    ui->treeWidget_2->clear();
    loadCategories();
    this->blockSignals(false);
}

void MainWindow::search(const QString &text)
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            return;
        }

        // Tokenize and normalize search text
        static const QRegularExpression whitespaceRe("\\s+");
        QStringList words = text.split(whitespaceRe, Qt::SkipEmptyParts);

        if (words.isEmpty()) {
            qDebug() << "No search terms provided.";
            return;
        }

        // Hash each token
        QStringList hashedTokens;
        for (const QString &word : std::as_const(words)) {
            QByteArray hash = QCryptographicHash::hash(word.toLower().toUtf8(),
                                                       QCryptographicHash::Sha256).toHex();
            hashedTokens << QString(hash);
        }

        // Build SQL
        QString placeholders = QString("?,").repeated(hashedTokens.size());
        placeholders.chop(1);

        QString sql =
            "SELECT DISTINCT a.id, a.category_id, a.application_name "
            "FROM application a "
            "JOIN application_tokens t ON a.id = t.application_id "
            "WHERE t.token_hash IN (" + placeholders + ")";

        QSqlQuery query(db);
        query.setForwardOnly(true);
        query.prepare(sql);

        for (const QString &h : std::as_const(hashedTokens)) {
            query.addBindValue(h);
        }

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id         = query.value(0).toInt();
            r.categoryId = query.value(1).toInt();
            r.appName    = DataObfuscator::deobfuscate(query.value(2).toString(), this->appKey);

            // Build full category path
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);

            // Optional: fetch last viewed timestamp for this app
            QSqlQuery q2(db);
            q2.prepare("SELECT MAX(dt) FROM application_views_audit WHERE application_id = ?");
            q2.addBindValue(r.id);
            if (q2.exec() && q2.next()) {
                qint64 unixTime = q2.value(0).toLongLong();
                if (unixTime > 0) {
                    QDateTime ts = QDateTime::fromSecsSinceEpoch(unixTime);
                    r.description = ts.toString(Qt::TextDate);
                }
            } else
            {
                showQueryError(this,query,Q_FUNC_INFO);
            }

            results.append(r);
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(this, tr("Search"), tr("No matches found."));
        ui->lineEditSearch->setFocus();
        ui->lineEditSearch->selectAll();
        return;
    }

    if (results.size() == 1) {
        selectInTreeWidgets(results.first().categoryId, results.first().id);
        const auto items = ui->treeWidget_2->selectedItems();
        if (items.isEmpty())
            return;  // nothing selected, nothing to open
        openPassword(items.first());
    } else {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Search Results"));

        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        QTableWidget *table = new QTableWidget(results.size(), 3, &dlg);
        table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Last Viewed") });
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->verticalHeader()->setDefaultSectionSize(22);

        for (int row = 0; row < results.size(); ++row) {
            QTableWidgetItem *appItem = new QTableWidgetItem(results[row].appName);
            appItem->setData(Qt::UserRole, results[row].id);
            appItem->setData(Qt::UserRole + 1, results[row].categoryId);
            table->setItem(row, 0, appItem);

            table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));
            table->setItem(row, 2, new QTableWidgetItem(results[row].description));
        }

        table->resizeColumnsToContents();
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSortingEnabled(true);
        layout->addWidget(table);

        QHBoxLayout *buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        QPushButton *cancelBtn = new QPushButton("Cancel", &dlg);
        QPushButton *okBtn     = new QPushButton("Select", &dlg);
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(okBtn);
        layout->addLayout(buttonLayout);

        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(table, &QTableWidget::itemDoubleClicked, &dlg, &QDialog::accept);

        QSize parentSize = this->size();
        int w = static_cast<int>(parentSize.width() * 0.75);
        int h = static_cast<int>(parentSize.height() * 0.75);
        QPoint parentPos = this->pos();
        int x = parentPos.x() + (parentSize.width() - w) / 2;
        int y = parentPos.y() + (parentSize.height() - h) / 2;
        dlg.setGeometry(x, y, w, h);

        if (dlg.exec() == QDialog::Accepted) {
            int row = table->currentRow();
            if (row >= 0) {
                QTableWidgetItem *item = table->item(row, 0);
                int appId      = item->data(Qt::UserRole).toInt();
                int categoryId = item->data(Qt::UserRole + 1).toInt();
                selectInTreeWidgets(categoryId, appId);
                const auto items = ui->treeWidget_2->selectedItems();
                if (items.isEmpty())
                    return;  // nothing selected, nothing to open
                openPassword(items.first());
            }
        }
    }

    ui->lineEditSearch->clear();
}

void MainWindow::selectInTreeWidgets(int categoryId, int appId)
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->data(0, Qt::UserRole).toInt() == categoryId) {
            ui->treeWidget->setCurrentItem(item);
            ui->treeWidget->scrollToItem(item);
            openCategory(item, 0); // column index doesn't matter here

            QTreeWidgetItemIterator it2(ui->treeWidget_2);
            while (*it2) {
                QTreeWidgetItem *childItem = *it2;
                if (childItem->data(0, Qt::UserRole).toInt() == appId) {
                    ui->treeWidget_2->setCurrentItem(childItem);
                    ui->treeWidget_2->scrollToItem(childItem);
                    break;
                }
                ++it2;
            }
            break;
        }
        ++it;
    }
}

QString MainWindow::buildItemPath(QTreeWidgetItem *item) const
{
    QStringList parts;
    QTreeWidgetItem *current = item;
    while (current) {
        parts.prepend(current->text(0)); // column 0 holds the name
        current = current->parent();
    }
    return parts.join(settings.getPathSeparator()); // or " > " if you prefer
}


void MainWindow::populateBookmarksMenu()
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            return;
        }

        // Clear existing actions if needed
        ui->menuBookmarks->clear();

        // Only show Recent and Popular if the number of results
        // of these queries has been set to > 0.
        if (settings.getMaxRecentResults()>0)
            ui->menuBookmarks->addAction(ui->actionRecent);
        if (settings.getMaxPopularResults()>0)
            ui->menuBookmarks->addAction(ui->actionPopular);

        if (!ui->menuBookmarks->isEmpty())
            ui->menuBookmarks->addSeparator();

        QSqlQuery query(db);
        query.prepare(R"(
            SELECT application.application_name,
                   application.id,
                   application.category_id
            FROM favourite
            INNER JOIN application
                ON favourite.application_id = application.id
            WHERE favourite.username = :username
        )");
        query.bindValue(":username", this->userName);
        query.setForwardOnly(true);

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            // Values from DB
            const QString appNameObf = query.value(0).toString();
            const int appId          = query.value(1).toInt();
            const int categoryId     = query.value(2).toInt();

            // Deobfuscated name for display
            const QString appName = DataObfuscator::deobfuscate(appNameObf, appKey);

            // Default status tip = app name; replace with full path if we find the category
            QString pathTip = appName;

            // Find the category node anywhere in ui->treeWidget
            QTreeWidgetItem *categoryItem = nullptr;
            for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
                categoryItem = findCategoryItemRecursive(ui->treeWidget->topLevelItem(i), categoryId);
                if (categoryItem) break;
            }

            if (categoryItem) {
                pathTip = buildItemPath(categoryItem) + settings.getPathSeparator() + appName;
            } else {
                qDebug() << "populateBookmarksMenu: categoryId" << categoryId << "not found in tree";
            }

            // Create the action
            QAction *action = new QAction(appName, this);
            action->setData(appId);
            action->setIcon(QPixmap(":/menus/glyphs/bookmark_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
            action->setStatusTip(pathTip);

            // Clicking runs your search by ID
            connect(action, &QAction::triggered, this, [this, appId]() {
                this->search(appId);
                const auto items = ui->treeWidget_2->selectedItems();
                if (items.isEmpty())
                    return;  // nothing selected, nothing to open

                openPassword(items.first());
            });

            ui->menuBookmarks->addAction(action);
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

// Helper: recursively search all children for a categoryId
QTreeWidgetItem* MainWindow::findCategoryItemRecursive(QTreeWidgetItem *item, int categoryId)
{
    if (!item) return nullptr;

    int idFromName = item->data(0,Qt::UserRole).toInt();
    if (idFromName == categoryId) {
        return item;
    }

    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *found = findCategoryItemRecursive(item->child(i), categoryId);
        if (found) return found;
    }

    return nullptr;
}


void MainWindow::search(int appId)
{
    // 1. Query DB for categoryId
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    int categoryId = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            return;
        }

        QSqlQuery query(db);
        query.prepare("SELECT category_id FROM application WHERE id = ?");
        query.addBindValue(appId);
        if (query.exec() && query.next()) {
            categoryId = query.value(0).toInt();
        } else
        {
            showQueryError(this,query,Q_FUNC_INFO);
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    if (categoryId == -1) {
        QMessageBox::information(this, tr("Search"), tr("Password not found in database."));
        return;
    }

    // 2. Walk the entire tree to find the category item
    QTreeWidgetItem *categoryItem = nullptr;
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> searchChildren =
        [&](QTreeWidgetItem* item) -> QTreeWidgetItem* {
        if (item->data(0,Qt::UserRole).toInt() == categoryId)
            return item;
        for (int i = 0; i < item->childCount(); ++i) {
            if (auto found = searchChildren(item->child(i)))
                return found;
        }
        return nullptr;
    };

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        if (auto found = searchChildren(ui->treeWidget->topLevelItem(i))) {
            categoryItem = found;
            break;
        }
    }

    if (!categoryItem) {
        QMessageBox::information(this, tr("Search"), tr("Category not found in tree."));
        return;
    }

    // 3. Highlight the category in ui->treeWidget
    ui->treeWidget->setCurrentItem(categoryItem);
    categoryItem->setSelected(true);
    ui->treeWidget->scrollToItem(categoryItem);

    // 4. Load the category into treeWidget_2
    openCategory(categoryItem, 0);

    // 5. Find and highlight the app in treeWidget_2
    for (int j = 0; j < ui->treeWidget_2->topLevelItemCount(); ++j) {
        QTreeWidgetItem *appItem = ui->treeWidget_2->topLevelItem(j);
        if (appItem->data(0,Qt::UserRole).toInt() == appId) {
            ui->treeWidget_2->setCurrentItem(appItem);
            appItem->setSelected(true);
            ui->treeWidget_2->scrollToItem(appItem);
            return;
        }
    }

    QMessageBox::information(this, tr("Search"), tr("Password not found in category."));
}

bool MainWindow::initDb()
{
    this->appKey = loadOrCreateAppKey();

    if (this->appKey.isEmpty()) {
        return false;   // signal failure
    }

    qApp->setProperty("appKey", this->appKey);

    initDbMetadata();
    checkGpgKeys();

    return true;
}

QByteArray MainWindow::loadOrCreateAppKey()
{

    //
    // 2. Fall back to DB-based key
    //
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QByteArray appKey;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            QMessageBox::critical(nullptr, QApplication::applicationName(), db.lastError().text());
            return {};
        }

        QSqlQuery query(db);
        query.prepare(R"(
            SELECT value
            FROM app_info
            WHERE key = :app_key
        )");
        query.bindValue(":app_key", "app_key");

        if (!query.exec()) {
            showQueryError(nullptr, query, Q_FUNC_INFO);
            return {};
        }

        if (query.next()) {

            QString value = query.value(0).toString().trimmed();

            //
            // CASE A — Key was exported → user must provide it manually
            //
            if (value.toLower() == "exported") {

                QApplication::restoreOverrideCursor();

                // --- Build inline dialog ---
                QDialog dlg(this);
                dlg.setWindowTitle(tr("Application Key Required"));
                dlg.setModal(true);
                dlg.resize(480, 240);

                QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
                QTabWidget *tabs = new QTabWidget(&dlg);

                //
                // TAB 1 — Paste Key
                //
                QWidget *pasteTab = new QWidget(&dlg);
                QVBoxLayout *pasteLayout = new QVBoxLayout(pasteTab);

                QLabel *pasteLabel = new QLabel(
                    tr("Paste your application key below:"), pasteTab);
                QTextEdit *pasteEdit = new QTextEdit(pasteTab);
                pasteEdit->setPlaceholderText(tr("application key here.."));
                pasteEdit->setTabChangesFocus(true);

                pasteLayout->addWidget(pasteLabel);
                pasteLayout->addWidget(pasteEdit);
                pasteTab->setLayout(pasteLayout);

                //
                // TAB 2 — Load From File
                //
                QWidget *fileTab = new QWidget(&dlg);
                QVBoxLayout *fileLayout = new QVBoxLayout(fileTab);

                QLabel *fileLabel = new QLabel(
                tr("Load your application key from a file:"), fileTab);

                QPushButton *fileButton = new QPushButton(tr("Choose File.."), fileTab);
                fileButton->setFixedWidth(120);   // Smaller button

                QLabel *fileLoadedLabel = new QLabel("", fileTab);
                fileLoadedLabel->setStyleSheet("color: #006000; font-style: italic;");

                fileLayout->addWidget(fileLabel);
                fileLayout->addWidget(fileButton);
                fileLayout->addWidget(fileLoadedLabel);
                fileLayout->addStretch();
                fileTab->setLayout(fileLayout);

                tabs->addTab(pasteTab, "Paste Key");
                tabs->addTab(fileTab, "Load From File");
                tabs->setTabIcon(0,QIcon(":/menus/glyphs/content_paste_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
                tabs->setTabIcon(1,QIcon(":/menus/glyphs/attach_file_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));

                mainLayout->addWidget(tabs);

                //
                // Dialog buttons
                //
                QDialogButtonBox *buttons = new QDialogButtonBox(
                    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                mainLayout->addWidget(buttons);

                QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
                    dlg.accept();
                });
                QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, [&]() {
                    dlg.reject();
                });

                //
                // File picker
                //
                QString loadedKey;
                QObject::connect(fileButton, &QPushButton::clicked, &dlg, [&]() {
                    QString path = QFileDialog::getOpenFileName(
                        &dlg, "Select Application Key File", QDir::homePath(), "All Files (*)");

                    if (!path.isEmpty()) {
                        QFile f(path);
                        if (f.open(QIODevice::ReadOnly)) {
                            loadedKey = QString::fromUtf8(f.readAll()).trimmed();

                            QFileInfo info(path);
                            fileLoadedLabel->setText("Loaded: " + info.fileName());
                        }
                    }
                });

                dlg.setFixedSize(dlg.size());
                pasteEdit->setFocus();

                //
                // Show dialog
                //
                if (dlg.exec() != QDialog::Accepted) {
                    this->close();   // closes MainWindow immediately
                    return {};       // signals failure to main.cpp
                }

                //
                // Extract key from whichever tab was used
                //
                QString keyText;

                if (tabs->currentIndex() == 0) {
                    keyText = pasteEdit->toPlainText().trimmed();
                } else {
                    keyText = loadedKey;
                }

                //
                // Validate (must decode properly, and match expected format)
                //

                // 1. Base64 decode (as before)
                QByteArray decoded = QByteArray::fromBase64(keyText.toUtf8());
                if (decoded.isEmpty()) {
                    QMessageBox::critical(this, "Invalid Application Key",
                                          "The Application Key you provided is invalid, incomplete or incorrect.");
                    qApp->quit();
                    return {};
                }

                // 2. Regex check on the decoded content (192 hex chars)
                QString decodedStr = QString::fromUtf8(decoded);

                QRegularExpression re("^[A-Fa-f0-9]{192}$");
                if (!re.match(decodedStr).hasMatch()) {
                    QMessageBox::critical(this, tr("Invalid Application Key"),
                                          tr("The Application Key you provided is invalid, incomplete or incorrect."));
                    qApp->quit();
                    return {};
                }

                // 3. Keep original behavior
                appKey = decoded;
            }
            //
            // CASE B — Key exists normally in DB
            //
            else {
                appKey = QByteArray::fromBase64(value.toUtf8());
            }
        }
        else {
            //
            // 3. No key found — generate a new one (UNCHANGED)
            //
            QString appKeyStr;
            for (int i = 0; i < 6; ++i) {
                QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
                uuidStr.remove('-');
                appKeyStr += uuidStr;
            }

            QByteArray encoded = appKeyStr.toUtf8().toBase64();

            query.prepare(R"(
                INSERT INTO app_info (key, value)
                VALUES (:app_key, :guid)
            )");
            query.bindValue(":app_key", "app_key");
            query.bindValue(":guid", encoded);

            if (!query.exec()) {
                showQueryError(nullptr, query, Q_FUNC_INFO);
                return {};
            }

            appKey = QByteArray::fromBase64(encoded);
        }
    }

    QSqlDatabase::removeDatabase(connName);

    qInfo().noquote() << "Using appKey:"
                      << (appKey.length() > 7
                              ? appKey.left(3) + "..." + appKey.right(4)
                              : appKey);

    return appKey;
}


// QByteArray MainWindow::loadOrCreateAppKey()
// {
//     //
//     // 1. Try file-based key first
//     //
//     QString keyFilePath = appKeyFilePath();

//     if (QFile::exists(keyFilePath)) {
//         QFile keyFile(keyFilePath);
//         keyFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
//         if (keyFile.open(QIODevice::ReadOnly)) {
//             QByteArray encoded = keyFile.readAll();
//             keyFile.close();

//             QByteArray decoded = QByteArray::fromBase64(encoded);
//             if (!decoded.isEmpty()) {
//                 qInfo().noquote() << "Using appKey from file:"
//                                   << (decoded.length() > 7
//                                           ? decoded.left(3) + "..." + decoded.right(4)
//                                           : decoded)
//                                   << "in" << keyFilePath;
//                 return decoded;
//             }
//         }
//     } else {
//         qInfo().noquote() << "No appKey file exists at" << keyFilePath;
//     }

//     //
//     // 2. Fall back to DB-based key
//     //
//     QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
//     QByteArray appKey;

//     {
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//         db.setDatabaseName(qApp->property("dbFile").toString());

//         if (!db.open()) {
//             showDbNotOpenError(this, db, Q_FUNC_INFO);
//             QMessageBox::critical(nullptr, QApplication::applicationName(), db.lastError().text());
//             return {};
//         }

//         QSqlQuery query(db);
//         query.prepare(R"(
//             SELECT value
//             FROM app_info
//             WHERE key = :app_key
//         )");
//         query.bindValue(":app_key", "app_key");

//         if (!query.exec()) {
//             showQueryError(nullptr, query, Q_FUNC_INFO);
//             return {};
//         }

//         if (query.next()) {
//             appKey = QByteArray::fromBase64(query.value(0).toString().toUtf8());
//         } else {
//             //
//             // 3. No key found — generate a new one
//             //
//             QString appKeyStr;
//             for (int i = 0; i < 6; ++i) {
//                 QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
//                 uuidStr.remove('-');
//                 appKeyStr += uuidStr;
//             }

//             QByteArray encoded = appKeyStr.toUtf8().toBase64();

//             query.prepare(R"(
//                 INSERT INTO app_info (key, value)
//                 VALUES (:app_key, :guid)
//             )");
//             query.bindValue(":app_key", "app_key");
//             query.bindValue(":guid", encoded);

//             if (!query.exec()) {
//                 showQueryError(nullptr, query, Q_FUNC_INFO);
//                 return {};
//             }

//             appKey = QByteArray::fromBase64(encoded);
//         }
//     }

//     QSqlDatabase::removeDatabase(connName);

//     qInfo().noquote() << "Using appKey from DB:"
//                       << (appKey.length() > 7
//                               ? appKey.left(3) + "..." + appKey.right(4)
//                               : appKey);

//     return appKey;
// }

void MainWindow::initDbMetadata()
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            QMessageBox::critical(this, QApplication::applicationName(), db.lastError().text());
            return;
        }

        QSqlQuery query(db);

        query.prepare(R"(
            INSERT OR IGNORE INTO app_info (key, value)
            VALUES (:db_create, :dt)
        )");
        query.bindValue(":db_create", "db_create");
        query.bindValue(":dt", QDateTime::currentDateTime());

        if (!query.exec()) {
            showQueryError(this, query, Q_FUNC_INFO);
        }
    }

    QSqlDatabase::removeDatabase(connName);
}


void MainWindow::checkGpgKeys()
{
    QList<KeyEntry> keys = fetchKeys();
    if (!keys.isEmpty())
        return;

    qWarning().noquote() << "No GPG Keys have been linked.";
    QApplication::restoreOverrideCursor();

    QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("No Keys Configured"),
                              tr("You must configure at least one GPG private key "
                                 "fingerprint before saving passwords.\n\n"
                                 "Would you like to add one now?"),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        keyList();
    } else {
        QMessageBox::information(this,
                                 tr("Keys Required"),
                                 tr("Without a configured key, you will not be able "
                                    "to create or edit encrypted passwords."));
    }
}

void MainWindow::setBookmark(bool checked)
{
    auto selected = ui->treeWidget_2->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, ui->actionBookmark->text(), tr("No item selected."));
        return;
    }

    int id = selected.front()->data(0,Qt::UserRole).toInt();
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            return;
        }

        QSqlQuery query(db);
        if (checked) {
            query.prepare("INSERT INTO favourite (application_id, username) "
                          "VALUES (:id, :user)");
        } else {
            query.prepare("DELETE FROM favourite "
                          "WHERE application_id = :id AND username = :user");
        }

        query.bindValue(":id", id);
        query.bindValue(":user", userName);

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
        }
    }

    QSqlDatabase::removeDatabase(connName);
}



void MainWindow::deletePassword(QTreeWidgetItem *item)
{
    // The keyword the user must type to confirm
    const QString confirmationKeyword = tr("DELETE");

    // Build a dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Confirm Deletion");

    QVBoxLayout layout(&dialog);

    QLabel label("This action will permanently delete the password.\n"
                 "To confirm, type \"" + confirmationKeyword + "\" below:");
    layout.addWidget(&label);

    QLineEdit lineEdit;
    layout.addWidget(&lineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted,
                     &dialog,   // context object
                     [&]() {
                         if (lineEdit.text() == confirmationKeyword) {
                             dialog.accept();
                         } else {
                             QMessageBox::warning(&dialog,
                                                  ui->actionDelete_Password->text(),
                                                  tr("You must type \"") + confirmationKeyword + tr("\" exactly to proceed."));
                         }
                     });

    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Show dialog and check result
    if (dialog.exec() == QDialog::Accepted) {
        // Perform the deletion here
        QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (!db.open()) {
                QMessageBox::critical(this, "", db.lastError().text());
                return;
            }

            if (!settings.verifyDeleteAllowed(db, this)) {
                QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
                return; // bail out early
            }

            // proceed with delete
            QSqlQuery pragma(db);
            pragma.exec("PRAGMA foreign_keys = ON;");

            QSqlQuery query(db);
            query.prepare("DELETE FROM application WHERE id = :id");
            query.bindValue(":id",item->data(0,Qt::UserRole).toInt());
            if (!query.exec())
            {
                showQueryError(this,query,Q_FUNC_INFO);
            } else
            {
                QTreeWidgetItem *parent = item->parent();
                if (parent) {
                    parent->removeChild(item);   // detach from parent
                } else {
                    int index = ui->treeWidget_2->indexOfTopLevelItem(item);
                    if (index != -1) {
                        ui->treeWidget_2->takeTopLevelItem(index);  // detach from top-level (the case with the current version)
                    }
                }
                delete item;
            }
        }
        QSqlDatabase::removeDatabase(connName);
    }
}



void MainWindow::deleteCategory(QTreeWidgetItem *item)
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, ui->actionDelete_Category->text(), db.lastError().text());
            return;
        }

        if (!settings.verifyDeleteAllowed(db, this)) {
            QMessageBox::warning(this, ui->actionDelete_Category->text(), tr("Delete not permitted."));
            return; // bail out early
        }

        // Enable foreign key enforcement
        QSqlQuery pragma(db);
        pragma.exec("PRAGMA foreign_keys = ON;");

        // Prepare delete
        QSqlQuery query(db);
        query.prepare("DELETE FROM categories WHERE id = :id");
        query.bindValue(":id", ui->treeWidget->selectedItems().first()->data(0,Qt::UserRole).toInt());

        if (!query.exec()) {
            QSqlError err = query.lastError();
            // Check for foreign key violation
            if (err.nativeErrorCode() == "1811" ||
                err.databaseText().contains("FOREIGN KEY constraint failed")) {
                QMessageBox::warning(this,
                                     ui->actionDelete_Category->text(),
                                     tr("This category cannot be deleted because items are still assigned to it."));
            } else {
                showQueryError(this,query,Q_FUNC_INFO);
            }
        } else {
            // Remove from UI
            delete ui->treeWidget->selectedItems().first();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}


void MainWindow::searchPopular()
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, ui->actionPopular->text(), db.lastError().text());
            return;
        }

        QString sql = QString(
            "SELECT a.id AS application_id, "
            "       a.category_id, "
            "       a.application_name, "
            "       COUNT(av.audit_id) AS view_count, "
            "       MAX(av.dt) AS last_viewed "
            "FROM application_views_audit av "
            "JOIN application a ON av.application_id = a.id "
            "GROUP BY a.id, a.category_id, a.application_name "
            "ORDER BY view_count DESC, last_viewed DESC "
            "LIMIT %1"
        ).arg(settings.getMaxPopularResults());

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            showQueryError(this, query, Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id         = query.value("application_id").toInt();
            r.categoryId = query.value("category_id").toInt();
            r.appName    = DataObfuscator::deobfuscate(
                               query.value("application_name").toString(), this->appKey);
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);
            r.description  = query.value("view_count").toString();
            results.append(r);
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(this, ui->actionPopular->text(),
                                 tr("No popular applications found."));
        return;
    }

    // --- NON-MODAL DIALOG ---
    QDialog *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(ui->actionPopular->text());

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *table = new QTableWidget(results.size(), 3, dlg);
    table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Views") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);
    table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    for (int row = 0; row < results.size(); ++row) {
        auto *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole,     results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        auto *viewsItem = new QTableWidgetItem(results[row].description);
        viewsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 2, viewsItem);
    }

    // Let the table size its columns to contents
    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(false);
    table->setSortingEnabled(false);

    layout->addWidget(table);

    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    auto *closeBtn = new QPushButton(tr("Close"), dlg);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    // Activate items without closing the dialog
    connect(table, &QTableWidget::itemDoubleClicked, this,
        [this, table](QTableWidgetItem *item) {
            int row       = item->row();
            auto *appItem = table->item(row, 0);
            if (!appItem)
                return;
            int appId      = appItem->data(Qt::UserRole).toInt();
            int categoryId = appItem->data(Qt::UserRole + 1).toInt();
            selectInTreeWidgets(categoryId, appId);
            const auto items = ui->treeWidget_2->selectedItems();
            if (items.isEmpty())
                return;  // nothing selected, nothing to open
            openPassword(items.first());
        });

    // --- EXPLICIT WIDTH CALCULATION (same fix as searchRecent) ---
    layout->activate();

    int totalWidth = table->verticalHeader()->width();
    for (int c = 0; c < table->columnCount(); ++c)
        totalWidth += table->columnWidth(c);

    totalWidth += table->frameWidth() * 2;

    QMargins m = layout->contentsMargins();
    totalWidth += m.left() + m.right();

    totalWidth += 16; // padding

    dlg->resize(totalWidth, dlg->sizeHint().height());
    // --- END WIDTH FIX ---

    dlg->show();
}


void MainWindow::searchRecent()
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, ui->actionRecent->text(), db.lastError().text());
            return;
        }

        QString sql = QString(
            "WITH RECURSIVE category_path(id, path) AS ( "
            "    SELECT id, text "
            "    FROM categories "
            "    WHERE parent_id IS NULL "
            "    UNION ALL "
            "    SELECT c.id, category_path.path || '/' || c.text "
            "    FROM categories c "
            "    JOIN category_path ON c.parent_id = category_path.id "
            ") "
            "SELECT a.id AS application_id, "
            "       a.category_id, "
            "       a.application_name, "
            "       cp.path AS category_name, "
            "       av.dt, av.user, av.host, av.action "
            "FROM application AS a "
            "JOIN category_path AS cp "
            "  ON cp.id = a.category_id "
            "JOIN application_views_audit AS av "
            "  ON av.application_id = a.id "
            "WHERE av.audit_id = ( "
            "   SELECT av2.audit_id "
            "   FROM application_views_audit av2 "
            "   WHERE av2.application_id = a.id "
            "   ORDER BY av2.dt DESC, av2.audit_id DESC "
            "   LIMIT 1 "
            ") "
            "ORDER BY av.dt DESC, av.audit_id DESC "
            "LIMIT %1;").arg(settings.getMaxRecentResults());

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id           = query.value("application_id").toInt();
            r.categoryId   = query.value("category_id").toInt();
            r.appName      = DataObfuscator::deobfuscate(
                                 query.value("application_name").toString(), this->appKey);
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);

            QString dt = query.value("dt").toString();
            r.description = dt;

            results.append(r);
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(this, ui->actionRecent->text(),
                                 tr("No recent applications found."));
        return;
    }

    // --- NON-MODAL DIALOG ---
    QDialog *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(ui->actionRecent->text());

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *table = new QTableWidget(results.size(), 3, dlg);
    table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Last Viewed") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);
    table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    for (int row = 0; row < results.size(); ++row) {
        auto *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole,     results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        qint64 unixTime = results[row].description.toLongLong();
        QDateTime ts = QDateTime::fromSecsSinceEpoch(unixTime);
        QString formatted = ts.toString(Qt::TextDate);
        table->setItem(row, 2, new QTableWidgetItem(formatted));
    }

    // Let the table size its columns to contents
    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(false);
    table->setSortingEnabled(false);

    layout->addWidget(table);

    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    auto *closeBtn = new QPushButton(tr("Close"), dlg);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    // Activate items without closing the dialog
    connect(table, &QTableWidget::itemDoubleClicked, this,
        [this, table](QTableWidgetItem *item) {
            int row       = item->row();
            auto *appItem = table->item(row, 0);
            if (!appItem)
                return;
            int appId      = appItem->data(Qt::UserRole).toInt();
            int categoryId = appItem->data(Qt::UserRole + 1).toInt();
            selectInTreeWidgets(categoryId, appId);

            const auto items = ui->treeWidget_2->selectedItems();
            if (items.isEmpty())
                return;  // nothing selected, nothing to open
            openPassword(items.first());
        });

    // --- EXPLICITLY RESIZE DIALOG WIDTH TO FIT TABLE ---
    // Make sure layout is aware of all children
    layout->activate();

    int totalWidth = table->verticalHeader()->width();
    for (int c = 0; c < table->columnCount(); ++c)
        totalWidth += table->columnWidth(c);

    totalWidth += table->frameWidth() * 2;

    QMargins m = layout->contentsMargins();
    totalWidth += m.left() + m.right();

    // A little extra padding so it doesn't look cramped
    totalWidth += 16;

    // Keep current height (Qt's default / platform size)
    dlg->resize(totalWidth, dlg->sizeHint().height());
    // --- END WIDTH RESIZE ---

    dlg->show();
}


QString MainWindow::buildCategoryPath(int categoryId, const QString &appKey, QSqlDatabase &db)
{
    QStringList parts;
    int currentId = categoryId;

    while (currentId > 0) {
        QSqlQuery q(db);
        q.prepare("SELECT parent_id, text FROM categories WHERE id = ?");
        q.addBindValue(currentId);
        if (!q.exec() || !q.next()) {
            // Break if the category is missing or query fails
            break;
        }

        // Deobfuscate the current segment (leaf to root)
        const QString obfText = q.value("text").toString();
        parts.prepend(DataObfuscator::deobfuscate(obfText, this->appKey));

        // Move to parent (handle NULL parent_id)
        const QVariant parentVar = q.value("parent_id");
        if (parentVar.isNull()) {
            break;
        }
        currentId = parentVar.toInt();
    }

    return parts.join(settings.getPathSeparator());
}

// void MainWindow::editPassword(QTreeWidgetItem *item)
// {
//     if (settings.getKillGpgAgent()) {
//         killGpgAgent();
//     }

//     QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
//     QByteArray data;

//     // --- Step 1: Retrieve encrypted data from DB ---
//     ui->statusbar->showMessage(tr("Reading database.."));
//     QApplication::processEvents();

//     {
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//         db.setDatabaseName(qApp->property("dbFile").toString());
//         if (db.open()) {
//             QSqlQuery query(db);
//             query.setForwardOnly(true);
//             query.prepare("SELECT data FROM application WHERE id = :id");
//             query.bindValue(":id", item->data(0,Qt::UserRole).toInt());
//             if (query.exec() && query.first()) {
//                 data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
//             } else {
//                 showQueryError(this,query,Q_FUNC_INFO);
//             }
//         } else {
//             qCritical().noquote() << Q_FUNC_INFO << db.lastError().text();
//             QMessageBox::critical(this,QApplication::applicationName(),"No database open.");
//             return;
//         }
//     }
//     QSqlDatabase::removeDatabase(connName);
//     ui->statusbar->clearMessage();

//     // --- Step 2: Decrypt asynchronously ---
//     ui->statusbar->showMessage(tr("Decrypting data.."));
//     QApplication::processEvents();

// auto decBuffer = QSharedPointer<QByteArray>::create();
// QProcess *gpg = new QProcess(this);
// gpg->setProcessChannelMode(QProcess::SeparateChannels);

// connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
//     gpg->write(data);
//     data.fill(0);
//     gpg->closeWriteChannel();
// });

// connect(gpg, &QProcess::readyReadStandardOutput, this, [gpg, decBuffer]() {
//     decBuffer->append(gpg->readAllStandardOutput());
// });

// connect(gpg,
//         QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//         this,
//         [this, gpg, item, decBuffer](int exitCode, QProcess::ExitStatus status) {
//             if (status == QProcess::NormalExit && exitCode == 0) {
//                 QByteArray decrypted_data = *decBuffer;  // use accumulated buffer
//                 if (!decrypted_data.isEmpty()) {
//                     ui->statusbar->clearMessage();

//                     // --- Step 3: Populate dialog with decrypted JSON ---
//                     QJsonDocument doc = QJsonDocument::fromJson(decrypted_data);
//                     QJsonObject obj = doc.object();

//                     NewPasswordDialog dlg(this);
//                     dlg.setWindowTitle("Edit Password");

//                     QList<KeyEntry> keys = fetchKeys();
//                     dlg.setKeys(keys);

//                     // Fill dialog fields from JSON
//                     dlg.AppName       = obj.value("private_name").toString();
//                     dlg.PublicAppName = item->text(0);
//                     dlg.Description   = obj.value("description").toString();
//                     dlg.URL           = obj.value("url").toString();
//                     dlg.openPassword();

//                     // --- Traverse credentials ---
//                     QJsonArray creds = obj.value("credentials").toArray();
//                     for (int i = 0; i < creds.size(); ++i) {
//                         QJsonObject credObj = creds.at(i).toObject();
//                         QString username  = credObj.value("username").toString();
//                         QString password  = credObj.value("password").toString();
//                         QString secretOpt = credObj.value("secretOtpCode").toString();
//                         int length        = credObj.value("length").toInt();

//                         qDebug().noquote() << "Credential:"
//                                  << "username=" << username
//                                  << "password=" << password
//                                  << "secretOtpCode=" << secretOpt
//                                  << "length=" << length;

//                         dlg.openCredentials(username, password, secretOpt, length);
//                     }

//                     // --- Traverse notes ---
//                     QJsonArray notes = obj.value("notes").toArray();
//                     for (const QJsonValue &val : std::as_const(notes)) {
//                         QJsonObject noteObj = val.toObject();
//                         QString body  = noteObj.value("content").toString();
//                         dlg.openNote(body);
//                     }

//                     if (dlg.exec() == QDialog::Accepted) {
//                         QByteArray newJson = dlg.toJson();

//                         QString baseDir = "/dev/shm";
//                         if (!QFileInfo::exists(baseDir) || !QFileInfo(baseDir).isWritable()) {
//                             baseDir = QDir::tempPath();
//                         }

//                         QString tempFile = baseDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".asc";

//                         QStringList args;
//                         const QStringList keys = dlg.getCheckedKeys();
//                         for (const QString &key : keys) {
//                             args << "--recipient" << key;
//                         }
//                         args << "--encrypt" << "--armor" << "--output" << tempFile;

//                         QProcess enc;
//                         enc.start("gpg", args);
//                         enc.waitForStarted();
//                         enc.write(newJson);
//                         enc.closeWriteChannel();
//                         enc.waitForFinished();

//                         QFile f(tempFile);
//                         if (f.open(QIODevice::ReadOnly)) {
//                             QByteArray newEncrypted = f.readAll();
//                             f.close();
//                             wipeFile(tempFile);
//                             {
//                                 QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","sqlEditPassword");
//                                 db.setDatabaseName(qApp->property("dbFile").toString());
//                                 if (db.open()) {
//                                     if (!db.transaction()) {
//                                         showTransactionError(this,db,Q_FUNC_INFO);
//                                     }
//                                     bool ok = true;

//                                     // Update application
//                                     QSqlQuery update(db);
//                                     update.prepare(R"(
//                                         UPDATE application
//                                         SET application_name = :name,
//                                             data = :data
//                                         WHERE id = :id)");
//                                     update.bindValue(":name", DataObfuscator::obfuscate(dlg.PublicAppName, appKey));
//                                     update.bindValue(":data", DataObfuscator::obfuscate(QString::fromUtf8(newEncrypted), appKey));
//                                     update.bindValue(":id", item->data(0, Qt::UserRole).toInt());
//                                     if (!update.exec()) {
//                                         qDebug() << "Update failed:" << update.lastError().text();
//                                         ok = false;
//                                     } else
//                                     {
//                                         ok = true;
//                                     }

//                                     if (ok) { if (!db.commit()) qCritical().noquote() << Q_FUNC_INFO << "Commit failed:" << db.lastError().text(); }
//                                     else { if (!db.rollback()) qDebug().noquote() << Q_FUNC_INFO << "Rollback failed:" << db.lastError().text(); }
//                                     if (ok)
//                                         insertAuditRow(item->data(0, Qt::UserRole).toInt(),
//                                                        userName,
//                                                        QSysInfo::machineHostName(),
//                                                        "EDITED");
//                                 }
//                                 db.close();
//                             }
//                             QSqlDatabase::removeDatabase("sqlEditPassword");
//                         }
//                     }
//                     decrypted_data.fill(0);
//                 }
//             }
//             gpg->deleteLater();
//             QApplication::restoreOverrideCursor();
//         });

//     // Handle stderr
//     connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg]() {
//         QByteArray errors = gpg->readAllStandardError();
//         if (!errors.isEmpty()) {
//             ui->statusbar->showMessage(QString::fromUtf8(errors),10000);
//             qCritical().noquote() << Q_FUNC_INFO << "GPG stderr:" << errors;
//         }
//     });

//     gpg->start("gpg", QStringList() << "--decrypt");
// }

void MainWindow::editPassword(QTreeWidgetItem *item)
{
    if (settings.getKillGpgAgent()) {
        killGpgAgent();
    }

    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QByteArray encdata;

    // --- Step 1: Retrieve encrypted data from DB ---
    ui->statusbar->showMessage(tr("Reading database.."));
    QApplication::processEvents();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (db.open()) {
            QSqlQuery query(db);
            query.setForwardOnly(true);
            query.prepare("SELECT data FROM application WHERE id = :id");
            query.bindValue(":id", item->data(0, Qt::UserRole).toInt());

            if (query.exec() && query.first()) {
                encdata = DataObfuscator::deobfuscate(
                              query.value(0).toString(), appKey
                          ).toUtf8();
            } else {
                showQueryError(this, query, Q_FUNC_INFO);
            }
        } else {
            qCritical().noquote() << Q_FUNC_INFO << db.lastError().text();
            QMessageBox::critical(this, QApplication::applicationName(),
                                  "No database open.");
            return;
        }
    }

    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt using shared helper ---
    ui->statusbar->showMessage(tr("Decrypting data.."));
    QApplication::processEvents();

    decryptWithGpg(
        encdata,

        // --- onSuccess ---
        [this, item](const QByteArray &json) {
            ui->statusbar->clearMessage();

            // --- Step 3: Populate dialog with decrypted JSON ---
            QJsonDocument doc = QJsonDocument::fromJson(json);
            QJsonObject obj = doc.object();

            NewPasswordDialog dlg(this);
            dlg.setWindowTitle("Edit Password");

            QList<KeyEntry> keys = fetchKeys();
            dlg.setKeys(keys);

            dlg.AppName       = obj.value("private_name").toString();
            dlg.PublicAppName = item->text(0);
            dlg.Description   = obj.value("description").toString();
            dlg.URL           = obj.value("url").toString();
            dlg.openPassword();

            // --- Traverse credentials ---
            QJsonArray creds = obj.value("credentials").toArray();
            for (int i = 0; i < creds.size(); ++i) {
                QJsonObject credObj = creds.at(i).toObject();
                dlg.openCredentials(
                    credObj.value("username").toString(),
                    credObj.value("password").toString(),
                    credObj.value("secretOtpCode").toString(),
                    credObj.value("length").toInt()
                );
            }

            // --- Traverse notes ---
            QJsonArray notes = obj.value("notes").toArray();
            for (int i = 0; i < notes.size(); ++i) {
                const QJsonValue &val = notes.at(i);
                dlg.openNote(val.toObject().value("content").toString());
            }

            // --- If user saves changes ---
            if (dlg.exec() == QDialog::Accepted) {
                QByteArray newJson = dlg.toJson();

                QString baseDir = "/dev/shm";
                if (!QFileInfo::exists(baseDir) || !QFileInfo(baseDir).isWritable()) {
                    baseDir = QDir::tempPath();
                }

                QString tempFile = baseDir + "/" +
                                   QUuid::createUuid().toString(QUuid::WithoutBraces) +
                                   ".asc";

                QStringList args;
                const QStringList keys = dlg.getCheckedKeys();
                for (const QString &key : keys) {
                    args << "--recipient" << key;
                }
                args << "--encrypt" << "--armor" << "--output" << tempFile;

                QProcess enc;
                enc.start("gpg", args);
                enc.waitForStarted();
                enc.write(newJson);
                enc.closeWriteChannel();
                enc.waitForFinished();

                QFile f(tempFile);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray newEncrypted = f.readAll();
                    f.close();
                    wipeFile(tempFile);

                    {
                        QSqlDatabase db = QSqlDatabase::addDatabase(
                                              "QSQLITE", "sqlEditPassword");
                        db.setDatabaseName(qApp->property("dbFile").toString());

                        if (db.open()) {
                            if (!db.transaction()) {
                                showTransactionError(this, db, Q_FUNC_INFO);
                            }

                            bool ok = true;

                            QSqlQuery update(db);
                            update.prepare(R"(
                                UPDATE application
                                SET application_name = :name,
                                    data = :data
                                WHERE id = :id)");
                            update.bindValue(":name",
                                DataObfuscator::obfuscate(dlg.PublicAppName, appKey));
                            update.bindValue(":data",
                                DataObfuscator::obfuscate(
                                    QString::fromUtf8(newEncrypted), appKey));
                            update.bindValue(":id",
                                item->data(0, Qt::UserRole).toInt());

                            if (!update.exec()) {
                                qDebug() << "Update failed:" << update.lastError().text();
                                ok = false;
                            }

                            if (ok) {
                                if (!db.commit())
                                    qCritical().noquote() << Q_FUNC_INFO
                                                          << "Commit failed:"
                                                          << db.lastError().text();
                            } else {
                                if (!db.rollback())
                                    qDebug().noquote() << Q_FUNC_INFO
                                                       << "Rollback failed:"
                                                       << db.lastError().text();
                            }

                            if (ok)
                                insertAuditRow(
                                    item->data(0, Qt::UserRole).toInt(),
                                    userName,
                                    QSysInfo::machineHostName(),
                                    "EDITED"
                                );
                        }
                        db.close();
                    }

                    QSqlDatabase::removeDatabase("sqlEditPassword");
                }
            }
        },

        // --- onMissingKey ---
        [this](const QString &err) {
            ui->statusbar->clearMessage();
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, tr("GPG Error"), err);

        },

        // --- onFailure ---
        [this](const QString &err) {
            ui->statusbar->showMessage(err);
            QApplication::restoreOverrideCursor();
        }
    );
}


// void MainWindow::exportPassword(QTreeWidgetItem *item)
// {
//     if (!item) return;

//     // --- WARNING prompt ---
//     QMessageBox msgBox(this);
//     msgBox.setIcon(QMessageBox::Warning);
//     msgBox.setWindowTitle(tr("Security Warning"));

//     msgBox.setText(
//         tr("<div style='line-height:135%; margin:4px 0 6px 0;'>"
//            "<b><span style='color:#8c0000;'>You are about to export your password data in unencrypted form.</span></b>"
//            "<div style='margin-top:8px;'>"
//            "<ul style='margin:0; padding-left:20px;'>"
//            "<li style='margin-bottom:6px;'>Anyone with access to the file can read your passwords.</li>"
//            "<li style='margin-bottom:6px;'>The exported file will <b>not</b> be protected by GPG or any other encryption.</li>"
//            "</ul>"
//            "</div>"
//            "<b>Only continue if you fully understand and accept the security risks.</b>"
//            "</div>")
//     );

//     msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
//     msgBox.button(QMessageBox::Yes)->setText(tr("Export Anyway"));
//     msgBox.button(QMessageBox::Cancel)->setText(tr("Cancel"));

//     QMessageBox::StandardButton reply =
//         static_cast<QMessageBox::StandardButton>(msgBox.exec());



//     if (reply != QMessageBox::Yes) return;

//     if (settings.getKillGpgAgent()) {
//         killGpgAgent();
//     }

//     // --- Step 1: Retrieve encrypted data from DB ---
//     QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
//     QByteArray data;
//     ui->statusbar->showMessage(tr("Reading database.."));
//     {
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//         db.setDatabaseName(qApp->property("dbFile").toString());
//         if (db.open()) {
//             QSqlQuery query(db);
//             query.prepare("SELECT data FROM application WHERE id = :id");
//             query.bindValue(":id", item->data(0, Qt::UserRole).toInt());
//             if (query.exec() && query.first()) {
//                 data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
//             } else {
//                 showQueryError(this,query,Q_FUNC_INFO);
//             }
//         } else {
//             qCritical().noquote() << "Database not opened." << db.lastError().text();
//             QMessageBox::critical(this,QApplication::applicationName(),"No database open.");
//         }
//     }
//     QSqlDatabase::removeDatabase(connName);
//     ui->statusbar->clearMessage();

//     // --- Step 2: Decrypt asynchronously ---
//     ui->statusbar->showMessage(tr("Decrypting data.."));
//     QApplication::processEvents();

//     auto decBuffer = QSharedPointer<QByteArray>::create();
//     QProcess *gpg = new QProcess(this);
//     gpg->setProcessChannelMode(QProcess::SeparateChannels);

//     connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
//         gpg->write(data);
//         data.fill(0);
//         gpg->closeWriteChannel();
//     });

//     connect(gpg, &QProcess::readyReadStandardOutput, this, [gpg, decBuffer]() {
//         decBuffer->append(gpg->readAllStandardOutput());
//     });

//     connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg]() {
//         QByteArray errors = gpg->readAllStandardError();
//         if (!errors.isEmpty()) {
//             ui->statusbar->showMessage(QString::fromUtf8(errors));
//             qCritical().noquote() << Q_FUNC_INFO << "GPG stderr:" << errors;
//         }
//     });

//     connect(gpg,
//             QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//             this,
//             [this, gpg, item, decBuffer](int exitCode, QProcess::ExitStatus status) {
//                 if (status == QProcess::NormalExit && exitCode == 0) {
//                     QByteArray decrypted_data = *decBuffer;
//                     if (!decrypted_data.isEmpty()) {
//                         ui->statusbar->clearMessage();

//                         // Build suggested filename: <item text>_<date>.json
//                         QString baseName = item->text(0);
//                         QString dateStr  = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
//                         QString suggestedName = QDir::homePath() + "/" + baseName + "_" + dateStr + ".json";

//                         QString fileName = QFileDialog::getSaveFileName(
//                             this,
//                             tr("Save Decrypted JSON"),
//                             suggestedName,
//                             tr("JSON Files (*.json);;All Files (*)")
//                             );

//                         if (!fileName.isEmpty()) {
//                             QFile outFile(fileName);
//                             if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//                                 outFile.write(decrypted_data);
//                                 outFile.close();

//                                 insertAuditRow(item->data(0, Qt::UserRole).toInt(),
//                                                userName,
//                                                QSysInfo::machineHostName(),
//                                                "EXPORTED");

//                                 QMessageBox::information(this, ui->actionExport_Password->text(),
//                                                          tr("Decrypted JSON saved to:\n") + fileName);

//                             } else {
//                                 QMessageBox::critical(this, ui->actionExport_Password->text(),
//                                                       tr("Could not open file for writing:\n") + fileName);
//                             }
//                         }

//                         decrypted_data.fill(0);
//                     }
//                     openedCredentialID = item->text(1).toInt();
//                 }
//                 gpg->deleteLater();
//                 QApplication::restoreOverrideCursor();
//                 QApplication::processEvents();
//             });

//     gpg->start("gpg", QStringList() << "--decrypt");
// }

void MainWindow::exportPassword(QTreeWidgetItem *item)
{
    if (!item) return;

    // --- WARNING prompt ---
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("Security Warning"));

    msgBox.setText(
        tr("<div style='line-height:135%; margin:4px 0 6px 0;'>"
           "<b><span style='color:#8c0000;'>You are about to export your password data in unencrypted form.</span></b>"
           "<div style='margin-top:8px;'>"
           "<ul style='margin:0; padding-left:20px;'>"
           "<li style='margin-bottom:6px;'>Anyone with access to the file can read your passwords.</li>"
           "<li style='margin-bottom:6px;'>The exported file will <b>not</b> be protected by GPG or any other encryption.</li>"
           "</ul>"
           "</div>"
           "<b>Only continue if you fully understand and accept the security risks.</b>"
           "</div>")
    );

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.button(QMessageBox::Yes)->setText(tr("Export Anyway"));
    msgBox.button(QMessageBox::Cancel)->setText(tr("Cancel"));

    if (msgBox.exec() != QMessageBox::Yes)
        return;

    if (settings.getKillGpgAgent()) {
        killGpgAgent();
    }

    // --- Step 1: Retrieve encrypted data from DB ---
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QByteArray encdata;

    ui->statusbar->showMessage(tr("Reading database.."));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT data FROM application WHERE id = :id");
            query.bindValue(":id", item->data(0, Qt::UserRole).toInt());

            if (query.exec() && query.first()) {
                encdata = DataObfuscator::deobfuscate(
                              query.value(0).toString(), appKey
                          ).toUtf8();
            } else {
                showQueryError(this, query, Q_FUNC_INFO);
            }
        } else {
            qCritical().noquote() << "Database not opened." << db.lastError().text();
            QMessageBox::critical(this, QApplication::applicationName(),
                                  "No database open.");
        }
    }

    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt using shared helper ---
    ui->statusbar->showMessage(tr("Decrypting data.."));
    QApplication::processEvents();

    decryptWithGpg(
        encdata,

        // --- onSuccess ---
        [this, item](const QByteArray &json) {
            ui->statusbar->clearMessage();

            QByteArray decrypted_data = json;
            if (decrypted_data.isEmpty())
                return;

            // Build suggested filename: <item text>_<date>.json
            QString baseName = item->text(0);
            QString dateStr  = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            QString suggestedName =
                QDir::homePath() + "/" + baseName + "_" + dateStr + ".json";

            QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Save Decrypted JSON"),
                suggestedName,
                tr("JSON Files (*.json);;All Files (*)")
            );

            if (!fileName.isEmpty()) {
                QFile outFile(fileName);
                if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    outFile.write(decrypted_data);
                    outFile.close();

                    insertAuditRow(item->data(0, Qt::UserRole).toInt(),
                                   userName,
                                   QSysInfo::machineHostName(),
                                   "EXPORTED");

                    QMessageBox::information(
                        this,
                        ui->actionExport_Password->text(),
                        tr("Decrypted JSON saved to:\n") + fileName
                    );

                } else {
                    QMessageBox::critical(
                        this,
                        ui->actionExport_Password->text(),
                        tr("Could not open file for writing:\n") + fileName
                    );
                }
            }

            decrypted_data.fill(0);
            openedCredentialID = item->text(1).toInt();

            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
        },

        // --- onMissingKey ---
        [this](const QString &err) {
            ui->statusbar->clearMessage();
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
            QMessageBox::critical(this, tr("GPG Error"), err);
        },

        // --- onFailure ---
        [this](const QString &err) {
            ui->statusbar->showMessage(err);
            QApplication::restoreOverrideCursor();
            QApplication::processEvents();
        }
    );
}

QString MainWindow::getItemPath(QTreeWidgetItem *item, int column)
{
    QStringList parts;
    QTreeWidgetItem *current = item;
    while (current) {
        parts.prepend(current->text(column));
        current = current->parent();
    }
    return parts.join(settings.getPathSeparator());
}

void MainWindow::moveCategory(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem) {
    if (!sourceItem || !targetItem) {
        qWarning() << "Invalid source or target item, ignoring drop action.";
        return;
    }

    const int appId       = sourceItem->data(0, Qt::UserRole).toInt();
    const int newCategory = targetItem->data(0, Qt::UserRole).toInt();

    if (appId <= 0 || newCategory <= 0) {
        qWarning() << "Invalid IDs, aborting move.";
        return;
    }

    if (settings.getDragDropPrompt()) {
        const QString msg = tr("Move entry \"%1\" to category \"%2\"?")
        .arg(sourceItem->text(0), targetItem->text(0));
        if (QMessageBox::question(this, tr("Confirm Move"), msg,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }

    const QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("Could not open database:\n%1").arg(db.lastError().text()));
        } else {
            QSqlQuery query(db);
            query.prepare("UPDATE application SET category_id = :cat WHERE id = :id");
            query.bindValue(":cat", newCategory);
            query.bindValue(":id", appId);

            if (!query.exec()) {
                showQueryError(this,query,Q_FUNC_INFO);
            } else {
                QTreeWidget *tree = ui->treeWidget_2;
                if (tree) {
                    auto sel = tree->selectedItems();
                    if (!sel.isEmpty()) {
                        QTreeWidgetItem *selected = sel.first();
                        if (selected && selected != sourceItem) {
                            delete selected;   // remove the original
                        }
                    }
                }
                delete sourceItem; // clean up the copy
                if (tree) tree->update();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
}


void MainWindow::importApplicationsFromFile(const QString &filePath)
{
    struct AuditEvent {
        int appId;
        QString userName;
        QString hostName;
        QString action;
    };

    QVector<AuditEvent> auditEvents;

    // 1) Validate file
    QFileInfo fi(filePath);
    if (!fi.exists() || fi.suffix().toLower() != "json") {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("Selected file is not a valid JSON file."));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("Could not open file:\n%1").arg(file.errorString()));
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    if (jsonData.isEmpty()) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("File is empty."));
        return;
    }

    // 2) Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("JSON parse error:\n%1").arg(parseError.errorString()));
        return;
    }
    if (!doc.isArray() && !doc.isObject()) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("File does not contain a valid JSON array or object."));
        return;
    }
    QJsonArray items = doc.isArray() ? doc.array() : QJsonArray{ doc.object() };

    // 3) Prompt for keys dynamically
    QList<KeyEntry> keys = fetchKeys();
    if (keys.isEmpty()) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("No encryption keys available."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Select Keys for Import"));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel(tr("Tick one or more keys to encrypt imported entries:"), &dlg);
    layout->addWidget(hint);

    QListWidget *listWidget = new QListWidget(&dlg);
    for (const KeyEntry &entry : std::as_const(keys)) {
        const QString display = entry.label.isEmpty()
        ? entry.key
        : QString("%1  (%2)").arg(entry.label, entry.key);
        QListWidgetItem *item = new QListWidgetItem(display, listWidget);
        item->setData(Qt::UserRole, entry.key);
        item->setCheckState(Qt::Unchecked);
    }
    layout->addWidget(listWidget);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return; // user cancelled
    }

    QStringList selectedKeys;
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedKeys << item->data(Qt::UserRole).toString();
        }
    }
    if (selectedKeys.isEmpty()) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("No keys selected for encryption."));
        return;
    }

    // 4) Resolve category_id
    QTreeWidgetItem *selectedItem = ui->treeWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("No category selected in the tree."));
        return;
    }
    int categoryId = selectedItem->data(0, Qt::UserRole).toInt();
    if (categoryId <= 0) {
        QMessageBox::critical(this, tr("Import Error"),
                              tr("Invalid category_id in selected tree item."));
        return;
    }

    // 5) DB connection + transaction (best-effort)
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open() || !db.transaction()) {
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("Could not open DB or start transaction:\n%1")
                                      .arg(db.lastError().text()));
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        QStringList errors;
        int importedCount = 0, skippedCount = 0;

        for (const QJsonValue &val : items) {
            if (!val.isObject()) {
                errors << tr("Skipping entry: not a JSON object.");
                skippedCount++;
                continue;
            }
            QJsonObject obj = val.toObject();

            // --- Schema validation ---
            if (!obj.contains("private_name") || !obj.value("private_name").isString()) {
                errors << tr("Skipping entry: private_name missing or not a string.");
                skippedCount++;
                continue;
            }
            QString publicAppName = obj.value("private_name").toString().trimmed();
            if (publicAppName.isEmpty()) {
                errors << tr("Skipping entry: private_name is empty.");
                skippedCount++;
                continue;
            }

            if (!obj.contains("credentials") || !obj.value("credentials").isArray()) {
                errors << tr("Skipping '%1': credentials missing or not an array.")
                .arg(publicAppName);
                skippedCount++;
                continue;
            }
            QJsonArray creds = obj.value("credentials").toArray();
            if (creds.isEmpty()) {
                errors << tr("Skipping '%1': credentials array is empty.")
                .arg(publicAppName);
                skippedCount++;
                continue;
            }

            // --- Notes normalization: accept both ["string", ...] and [{content: "..."}] ---
            if (obj.contains("notes")) {
                QJsonValue notesVal = obj.value("notes");
                if (notesVal.isArray()) {
                    QJsonArray notesIn = notesVal.toArray();
                    QJsonArray notesOut;
                    bool notesOk = true;
                    for (const QJsonValue &nv : std::as_const(notesIn)) {
                        if (nv.isString()) {
                            notesOut.append(QJsonObject{{"content", nv.toString()}});
                        } else if (nv.isObject()) {
                            QJsonObject nobj = nv.toObject();
                            if (nobj.contains("content") && nobj.value("content").isString()) {
                                // keep as-is
                                notesOut.append(QJsonObject{{"content", nobj.value("content").toString()}});
                            } else {
                                notesOk = false;
                                break;
                            }
                        } else {
                            notesOk = false;
                            break;
                        }
                    }
                    if (!notesOk) {
                        errors << tr("Skipping '%1': notes entries must be strings or objects with 'content'.")
                        .arg(publicAppName);
                        skippedCount++;
                        continue;
                    }
                    obj["notes"] = notesOut; // normalized to [{content: "..."}]
                } else if (notesVal.isString()) {
                    // single string -> wrap
                    obj["notes"] = QJsonArray{ QJsonObject{{"content", notesVal.toString()}} };
                } else if (notesVal.isNull()) {
                    obj["notes"] = QJsonArray(); // treat null as empty array
                } else {
                    errors << tr("Skipping '%1': notes must be an array or string.")
                    .arg(publicAppName);
                    skippedCount++;
                    continue;
                }
            } else {
                // Ensure 'notes' exists as an empty array
                obj["notes"] = QJsonArray();
            }

            // Optional: ensure description/url types (non-fatal)
            if (obj.contains("description") && !obj.value("description").isString()) {
                obj["description"] = QJsonValue(QString()); // normalize to empty string
            }
            if (obj.contains("url") && !obj.value("url").isString()) {
                obj["url"] = QJsonValue(QString());
            }

            QByteArray plainJson = QJsonDocument(obj).toJson(QJsonDocument::Compact);
            if (plainJson.isEmpty()) {
                errors << tr("Skipping '%1': serialization failed.")
                .arg(publicAppName);
                skippedCount++;
                continue;
            }

            // --- Encrypt with GPG ---
            QString tempFile = QDir::tempPath() + "/import.asc";
            QStringList args;
            for (const QString &key : std::as_const(selectedKeys)) {
                args << "--recipient" << key;
            }
            args << "--encrypt" << "--armor" << "--output" << tempFile;

            QProcess process;
            process.start("gpg", args);
            if (!process.waitForStarted(5000)) {
                errors << tr("Skipping '%1': GPG failed to start.").arg(publicAppName);
                skippedCount++;
                continue;
            }
            process.write(plainJson);
            process.closeWriteChannel();
            if (!process.waitForFinished(15000)) {
                errors << tr("Skipping '%1': GPG did not finish.").arg(publicAppName);
                skippedCount++;
                continue;
            }
            const QByteArray gpgErr = process.readAllStandardError();
            if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
                errors << tr("Skipping '%1': GPG error: %2")
                .arg(publicAppName, QString::fromUtf8(gpgErr));
                skippedCount++;
                continue;
            }

            QFile outFile(tempFile);
            if (!outFile.open(QIODevice::ReadOnly)) {
                errors << tr("Skipping '%1': failed to read encrypted output.")
                .arg(publicAppName);
                skippedCount++;
                continue;
            }
            QByteArray encrypted = outFile.readAll();
            outFile.close();
            wipeFile(tempFile);

            if (encrypted.isEmpty()) {
                errors << tr("Skipping '%1': encrypted output is empty.")
                .arg(publicAppName);
                skippedCount++;
                continue;
            }

            // --- Insert application ---
            QSqlQuery query(db);
            query.prepare(R"(
                INSERT INTO application (category_id, application_name, data, created)
                VALUES (:category_id, :application_name, :data, :created))");
            query.bindValue(":category_id", categoryId);
            query.bindValue(":application_name", DataObfuscator::obfuscate(publicAppName, this->appKey));
            query.bindValue(":data", DataObfuscator::obfuscate(QString::fromUtf8(encrypted), appKey));
            query.bindValue(":created", QDateTime::currentSecsSinceEpoch());

            if (!query.exec()) {
                errors << tr("Insert failed for '%1': %2")
                .arg(publicAppName, query.lastError().text());
                skippedCount++;
                continue;
            }

            int appId = query.lastInsertId().toInt();

            auditEvents.append({
                appId,
                userName,
                QSysInfo::machineHostName(),
                "CREATED"
            });

            // --- Tokenize ---
            static const QRegularExpression whitespaceRe("\\s+");
            QStringList tokens = publicAppName.toLower().split(whitespaceRe, Qt::SkipEmptyParts);
            bool tokenOk = true;
            for (const QString &token : std::as_const(tokens)) {
                QByteArray hash = QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256).toHex();
                QSqlQuery insertToken(db);
                insertToken.prepare("INSERT INTO application_tokens (application_id, token_hash) VALUES (?, ?)");
                insertToken.addBindValue(appId);
                insertToken.addBindValue(QString(hash));
                if (!insertToken.exec()) {
                    errors << tr("Token insert failed for '%1': %2")
                    .arg(publicAppName, insertToken.lastError().text());
                    tokenOk = false;
                    break;
                }
            }
            if (!tokenOk) {
                skippedCount++;
                continue;
            }

            importedCount++;
        }

        // Commit
        if (!db.commit()) {
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("Commit failed:\n%1").arg(db.lastError().text()));
            db.rollback();
        } else {

            // Insert audit rows AFTER commit to avoid SQLite writer lock
            for (const AuditEvent &ev : auditEvents) {
                insertAuditRow(ev.appId, ev.userName, ev.hostName, ev.action);
            }

            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setWindowTitle(tr("Import Complete"));
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setText(
                tr("Imported: %1<br>Skipped: %2<br><br>"
                   "<b><span style='color:red;'>WARNING, WARNING, WARNING</span></b><br><br>"
                   "The JSON file you just imported will remain in UNENCRYPTED form.<br><br>"
                   "Leaving plaintext password files around is a serious security risk. "
                   "Please securely delete or move the import file immediately to prevent leaks.")
                    .arg(importedCount)
                    .arg(skippedCount)
                );
            msgBox.exec();
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    //refresh the view
    // imported passwords show immediately
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (current) {
        this->openCategory(current, 0);
    }
}

void MainWindow::createCategory(const QString& categoryName /* = QString() */)
{
    QString text = categoryName.trimmed();
    bool forceTopLevel = false;

    //
    // 1. Show dialog if no name was passed in
    //
    if (text.isEmpty()) {
        int existingCount = 0;
        if (ui->treeWidget->topLevelItemCount() > 0 &&
            ui->treeWidget->currentItem() != nullptr)
        {
            existingCount = ui->treeWidget->topLevelItemCount();
        }
        CategoryDialog dlg(this,existingCount);
        if (dlg.exec() != QDialog::Accepted)
            return;

        text = dlg.categoryName();
        if (text.isEmpty())
            return;

        forceTopLevel = dlg.isTopLevel();
    }

    //
    // 2. Validate name
    //
    static const QRegularExpression re("^.{1,64}$");
    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this,
                             tr("Invalid Name"),
                             tr("Category name contains invalid characters or is too long."));
        return;
    }

    //
    // 3. Determine parent item and parent_id
    //
    QTreeWidgetItem* parentItem = nullptr;
    QVariant parentId;   // NULL by default → top-level

    if (!forceTopLevel) {
        parentItem = ui->treeWidget->currentItem();
        if (parentItem) {
            parentId = parentItem->data(0, Qt::UserRole);
        }
    }

    //
    // 4. Duplicate check among siblings
    //
    QTreeWidgetItem* scope = parentItem
                                 ? parentItem
                                 : ui->treeWidget->invisibleRootItem();

    for (int i = 0; i < scope->childCount(); ++i) {
        if (scope->child(i)->text(0) == text) {
            QMessageBox::warning(this, tr("Duplicate"),
                                 tr("A category with this name already exists here."));
            return;
        }
    }

    //
    // 5. Insert into DB
    //
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            qCritical().noquote() << Q_FUNC_INFO << "No open database." << db.lastError().text();
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("No open database.\n%1").arg(db.lastError().text()));
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        QSqlQuery query(db);
        query.prepare("INSERT INTO categories (parent_id, text) VALUES (:parent_id, :text)");
        query.bindValue(":parent_id", parentId);  // NULL for top-level
        query.bindValue(":text", DataObfuscator::obfuscate(text, this->appKey));

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            db.close();
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        int newId = query.lastInsertId().toInt();

        //
        // 6. Create tree item
        //
        QTreeWidgetItem* newItem = new QTreeWidgetItem();
        newItem->setText(0, text);
        newItem->setIcon(0, QIcon(":/menus/glyphs/folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        newItem->setData(0, Qt::UserRole, newId);

        if (parentItem) {
            parentItem->addChild(newItem);
            parentItem->setExpanded(true);
        } else {
            ui->treeWidget->addTopLevelItem(newItem);
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connName);
}


void MainWindow::renameCategory()
{
    QTreeWidgetItem* item = ui->treeWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a category to rename."));
        return;
    }

    QString oldText = item->text(0);

    // Ask for new name, pre-filled with current
    QString text = QInputDialog::getText(
                       this,
                       tr("Rename Category"),
                       tr("Enter a new name:"),
                       QLineEdit::Normal,
                       oldText
                       ).trimmed();

    if (text.isEmpty() || text == oldText) {
        return; // cancelled or unchanged
    }

    //validate
    static const QRegularExpression re("^[\\w\\-\\s]{1,64}$");
    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Name"),
                             tr("Category name contains invalid characters or is too long."));
        return;
    }


    // Duplicate check among siblings
    QTreeWidgetItem* parentItem = item->parent();
    QTreeWidgetItem* scope = parentItem ? parentItem : ui->treeWidget->invisibleRootItem();
    for (int i = 0; i < scope->childCount(); ++i) {
        QTreeWidgetItem* sibling = scope->child(i);
        if (sibling != item && sibling->text(0) == text) {
            QMessageBox::warning(this, tr("Duplicate"), tr("A category with this name already exists here."));
            return;
        }
    }

    // DB connection
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            qCritical().noquote() << Q_FUNC_INFO << "No open database." << db.lastError().text();
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("No open database.\n%1").arg(db.lastError().text()));
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        int id = item->data(0, Qt::UserRole).toInt();
        QSqlQuery query(db);
        query.prepare("UPDATE categories SET text = :text WHERE id = :id");
        query.bindValue(":text", DataObfuscator::obfuscate(text, this->appKey));
        query.bindValue(":id", id);

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        } else
        {
            // Update UI
            item->setText(0, text);
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

void MainWindow::addSearchTerms(QTreeWidgetItem *item)
{
    // Find the currently selected application in your tree
    int appId = item->data(0, Qt::UserRole).toInt();
    if (appId <= 0) {
        QMessageBox::warning(this, tr("Add Search Term"), tr("Invalid application selection."));
        return;
    }

    // Build dialog
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add Search Terms"));

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLineEdit *lineEdit = new QLineEdit(&dlg);
    lineEdit->setPlaceholderText(tr("Enter search term(s)..."));
    layout->addWidget(lineEdit);

    QListWidget *listWidget = new QListWidget(&dlg);
    layout->addWidget(listWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    QPushButton *addBtn = new QPushButton(tr("Add"), &dlg);
    QPushButton *okBtn  = new QPushButton(tr("Save"), &dlg);
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    // Helper lambda to check duplicates
    auto isDuplicate = [listWidget](const QString &text) {
        QString t = text.trimmed().toLower();
        for (int i = 0; i < listWidget->count(); ++i) {
            if (listWidget->item(i)->text().trimmed().toLower() == t)
                return true;
        }
        return false;
    };

    connect(addBtn, &QPushButton::clicked,
            addBtn,
            [lineEdit, listWidget, isDuplicate]() {
                QString text = lineEdit->text().trimmed();
                if (!text.isEmpty() && !isDuplicate(text)) {
                    listWidget->addItem(text);
                }
                lineEdit->clear();
            });

    connect(okBtn, &QPushButton::clicked,
            &dlg, [lineEdit, listWidget, &dlg, isDuplicate]() {
                QString text = lineEdit->text().trimmed();
                if (!text.isEmpty() && !isDuplicate(text)) {
                    listWidget->addItem(text);
                }
                lineEdit->clear();
                dlg.accept();
            });

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        bool success = true;
        for (int i = 0; i < listWidget->count(); ++i) {
            QString term = listWidget->item(i)->text();
            static const QRegularExpression whitespaceRe("\\s+");
            QStringList tokens = term.toLower().split(whitespaceRe, Qt::SkipEmptyParts);
            for (const QString &token : std::as_const(tokens)) {
                QByteArray hash = QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256).toHex();
                QSqlQuery insertToken(db);
                insertToken.prepare("INSERT INTO application_tokens (application_id, token_hash) VALUES (?, ?)");
                insertToken.addBindValue(appId);
                insertToken.addBindValue(QString(hash));
                if (!insertToken.exec()) {
                    qDebug() << "Token insert failed:" << insertToken.lastError().text();
                    success = false;
                    break;
                }
            }
            if (!success) break;
        }

        QSqlDatabase::removeDatabase(connName);

        if (success) {
            QMessageBox::information(this, tr("Add Search Term"), tr("Search terms added successfully."));
        } else {
            QMessageBox::warning(this, tr("Add Search Term"), tr("Failed to add one or more search terms."));
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // ESC Key
    if (event->key() == Qt::Key_Escape) {
        if (openedCredentialID == -1) {
            if (settings.getAskClose()) {
                close();
            }
            event->accept();
            return;
        } else {
            clearScrollArea();
            event->accept();
            return;
        }
    }

    // CTRL+F
    if (event->key() == Qt::Key_F && event->modifiers() == Qt::ControlModifier) {
        ui->lineEditSearch->setFocus();
        event->accept();
        return;
    }

    // F2 — open new password dialog
    if (event->key() == Qt::Key_F2) {
        newPassword();
        event->accept();
        return;
    }

    // Delete — delete category or password depending on focus
    if (event->key() == Qt::Key_Delete) {
        qDebug() << "Got Delete Key";

        QWidget *focus = QApplication::focusWidget();

        if (focus == ui->treeWidget) {
            deleteCategory(ui->treeWidget->currentItem());
            event->accept();
            return;
        }

        if (focus == ui->treeWidget_2) {
            deletePassword(ui->treeWidget_2->currentItem());
            event->accept();
            return;
        }

        // If Delete was pressed but no relevant widget had focus
        event->accept();
        return;
    }

    // Pass unhandled keys to base class
    QMainWindow::keyPressEvent(event);
}

void MainWindow::launchHelperProcess(const QString &page)
{
    if (!hasHelp())
    {
        QMessageBox::warning(
            this,
            QApplication::applicationName(),
            tr("No help available.\n\n"
               "You can either allow access to %1 or download offline help from %2.")
                .arg(Passwords::HelpBaseUrl,
                     Passwords::GitUrl)
        );
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                      + QDir::separator()
                      + QCoreApplication::applicationName()
                      + QCoreApplication::applicationVersion();
    QDir().mkpath(tempDir);

    QString exePath = QCoreApplication::applicationDirPath() + QDir::separator()
                      + QStringLiteral("pwdhlp")
#if defined(Q_OS_WIN)
                      + QStringLiteral(".exe")
#endif
        ;

    QStringList args;
    args << "--webdir" << tempDir
         << "--port"   << QString::number(settings.getHelpPort());

    if (!page.isEmpty()) {
        args << "--page" << page;
    }

    if (!helperProcess || helperProcess->state() == QProcess::NotRunning) {
        // First time: track the process
        helperProcess = new QProcess(this);
        helperProcess->setProgram(exePath);
        helperProcess->setArguments(args);

        connect(helperProcess, &QProcess::errorOccurred, this,
                [this, exePath](QProcess::ProcessError error){
                    if (helperProcess->state() == QProcess::Starting &&
                        error == QProcess::FailedToStart) {
                        QMessageBox::warning(nullptr, tr("Help"),
                                             QString(tr("Failed to start: %1")).arg(exePath));
                    }
                });

        helperProcess->start();
    } else {
        // Already running: just launch another instance detached
        QProcess::startDetached(exePath, args);
    }

    QApplication::restoreOverrideCursor();
}

QString MainWindow::formatOtp(const QString& otp)
{
    const int len = otp.length();
    if (len == 0)
        return QString();

    QString formatted;
    formatted.reserve(len + len / 3); // small extra space for dashes

    QStringView view{otp};

    int firstGroupSize = 0;

    if (len % 2 == 0) {
        // Even length: split into two equal halves
        firstGroupSize = len / 2;      // e.g. 6 -> 3, 8 -> 4
    } else {
        // Odd length: make first group the remainder when divided by 3
        const int rem = len % 3;       // 1 or 2, or 0
        firstGroupSize = (rem == 0) ? 3 : rem;
    }

    // Add first group
    formatted = view.left(firstGroupSize).toString();

    // Add remaining groups of 3 with '-' separators
    int index = firstGroupSize;
    while (index < len) {
        formatted += '-';
        const int chunkSize = qMin(3, len - index);

        // Use QStringView::mid here – no QString::mid(), no clazy warning
        formatted += view.mid(index, chunkSize);

        index += chunkSize;
    }

    return formatted;
}

void MainWindow::wipeFile(const QString &path, int passes)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open file for wiping:" << path;
        return;
    }

    qint64 size = f.size();
    const qint64 chunkSize = 4096;
    QByteArray buffer(chunkSize, '\0');

    for (int pass = 0; pass < passes; ++pass) {
        qDebug() << "Wiping pass" << (pass + 1) << "on" << f.fileName();

        if (!f.seek(0)) {
            qWarning() << "Failed to seek to beginning of file:" << f.fileName();
            break;
        }

        qint64 remaining = size;
        while (remaining > 0) {
            qint64 toWrite = qMin(chunkSize, remaining);

            if (pass % 2 == 0) {
                // Random data pass
                for (qint64 i = 0; i < toWrite; ++i) {
                    buffer[i] = static_cast<char>(
                        QRandomGenerator::global()->bounded(256));
                }
            } else {
                // Zero pass
                buffer.fill('\0', toWrite);
            }

            if (f.write(buffer.constData(), toWrite) != toWrite) {
                qWarning() << "Failed to overwrite chunk in" << f.fileName();
                break;
            }

            remaining -= toWrite;
        }

        f.flush();
    }

    f.close();

    if (!QFile::remove(path)) {
        qWarning() << "Failed to remove file:" << path;
    } else {
        qDebug() << "File securely wiped and removed:" << path;
    }
}

void MainWindow::insertAuditRow(int applicationId, const QString &user, const QString &host, const QString &action)
{
    const QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (db.open()) {

            {
                QSqlQuery audit(db);
                if (!audit.prepare(R"(
                    INSERT INTO application_views_audit(
                        application_id, dt, user, host, action, audit_time
                    ) VALUES (
                        :id, :dt, :user, :host, :action, strftime('%s','now')
                    )
                )")) {
                    qWarning() << "Prepare failed:" << audit.lastError();
                }

                audit.bindValue(":id", applicationId);
                audit.bindValue(":dt", QDateTime::currentSecsSinceEpoch());
                audit.bindValue(":user", user);
                audit.bindValue(":host", host);
                audit.bindValue(":action", action);

                if (!audit.exec())
                    qWarning() << "Audit insert failed:" << audit.lastError();
            }
        }
    }

    QSqlDatabase::removeDatabase(connName);
}

void MainWindow::ExportedWithoutEdits()
{
    const QString dbFile = qApp->property("dbFile").toString();
    if (Q_UNLIKELY(dbFile.isEmpty())) {
        QMessageBox::warning(this, ui->actionKey_List->text(), tr("No database open."));
        return;
    }

    qDebug() << countExportedWithoutEdits();
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, tr("Exported Passwords"), db.lastError().text());
            return;
        }

        QString sql =
            "WITH last_export AS ("
            "    SELECT application_id, MAX(dt) AS last_export_dt "
            "    FROM application_views_audit "
            "    WHERE action = 'EXPORTED' "
            "    GROUP BY application_id"
            ") "
            "SELECT a.id AS application_id, "
            "       a.category_id, "
            "       a.application_name, "
            "       le.last_export_dt "
            "FROM application a "
            "JOIN last_export le ON le.application_id = a.id "
            "WHERE NOT EXISTS ("
            "    SELECT 1 "
            "    FROM application_views_audit av "
            "    WHERE av.application_id = a.id "
            "      AND av.action = 'EDITED' "
            "      AND av.dt > le.last_export_dt"
            ");";

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id         = query.value("application_id").toInt();
            r.categoryId = query.value("category_id").toInt();
            r.appName    = DataObfuscator::deobfuscate(query.value("application_name").toString(), this->appKey);
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);
            r.description = query.value("last_export_dt").toString();
            results.append(r);
        }
    }

    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(this, tr("Exported Passwords"), "No exported passwords without edits found.");
        return;
    }

    // -------------------------
    // Build MODELLESS dialog UI
    // -------------------------
    QDialog *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Exported Passwords Without Edits"));

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QTableWidget *table = new QTableWidget(results.size(), 3, dlg);
    table->setHorizontalHeaderLabels({ tr("Password"), tr("Category"), tr("Exported") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);

    // 🔥 Prevent Qt from auto‑closing the dialog on double‑click
    disconnect(table, &QTableWidget::itemActivated, nullptr, nullptr);

    for (int row = 0; row < results.size(); ++row) {

        QTableWidgetItem *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole, results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        qint64 ts = results[row].description.toLongLong();
        QDateTime dt = QDateTime::fromSecsSinceEpoch(ts);
        QString shortDate = QLocale().toString(dt, QLocale::ShortFormat);

        QTableWidgetItem *exportedItem = new QTableWidgetItem(shortDate);
        exportedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 2, exportedItem);
    }

    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSortingEnabled(false);
    layout->addWidget(table);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("Cancel", dlg);
    QPushButton *okBtn     = new QPushButton("Select", dlg);
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(okBtn);
    layout->addLayout(buttonLayout);

    // -------------------------
    // Modeless behavior wiring
    // -------------------------

    auto selectRow = [this, dlg, table]() {
        int row = table->currentRow();
        if (row >= 0) {
            QTableWidgetItem *item = table->item(row, 0);
            int appId      = item->data(Qt::UserRole).toInt();
            int categoryId = item->data(Qt::UserRole + 1).toInt();
            selectInTreeWidgets(categoryId, appId);
        }
    };

    connect(okBtn, &QPushButton::clicked, dlg, [selectRow, dlg]() {
        selectRow();
        dlg->close();
    });

    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::close);

    connect(table, &QTableWidget::itemDoubleClicked, dlg, [selectRow]() {
        selectRow();
        // ❗ Do NOT close the dialog — user wants it to stay open
    });

    // Center dialog
    QSize parentSize = this->size();
    int w = static_cast<int>(parentSize.width() * 0.75);
    int h = static_cast<int>(parentSize.height() * 0.75);
    QPoint parentPos = this->pos();
    int x = parentPos.x() + (parentSize.width() - w) / 2;
    int y = parentPos.y() + (parentSize.height() - h) / 2;
    dlg->setGeometry(x, y, w, h);

    // --- EXPLICIT WIDTH CALCULATION (same fix as searchRecent) ---
    layout->activate();

    int totalWidth = table->verticalHeader()->width();
    for (int c = 0; c < table->columnCount(); ++c)
        totalWidth += table->columnWidth(c);

    totalWidth += table->frameWidth() * 2;

    QMargins m = layout->contentsMargins();
    totalWidth += m.left() + m.right();

    totalWidth += 16; // padding

    dlg->resize(totalWidth, dlg->sizeHint().height());
    // --- END WIDTH FIX ---

    dlg->show();   // MODELLESS
}

int MainWindow::countExportedWithoutEdits()
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    int count = 0;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            qCritical() << "DB open failed:" << db.lastError().text();
            return 0;
        }

        QString sql =
            "WITH last_export AS ("
            "    SELECT application_id, MAX(dt) AS last_export_dt "
            "    FROM application_views_audit "
            "    WHERE action = 'EXPORTED' "
            "    GROUP BY application_id"
            ") "
            "SELECT COUNT(*) "
            "FROM application a "
            "JOIN last_export le ON le.application_id = a.id "
            "WHERE NOT EXISTS ("
            "    SELECT 1 "
            "    FROM application_views_audit av "
            "    WHERE av.application_id = a.id "
            "      AND av.action = 'EDITED' "
            "      AND av.dt > le.last_export_dt"
            ");";

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            showQueryError(this,query,Q_FUNC_INFO);
            return 0;
        }

        if (query.next()) {
            count = query.value(0).toInt();
        }
    }

    QSqlDatabase::removeDatabase(connName);
    return count;
}

void MainWindow::NotChangedSince(const QDateTime &cutoff)
{
    const QString dbFile = qApp->property("dbFile").toString();
    if (Q_UNLIKELY(dbFile.isEmpty())) {
        QMessageBox::warning(this, ui->actionKey_List->text(), tr("No database open."));
        return;
    }

    qint64 cutoffTs = cutoff.toSecsSinceEpoch();

    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            showDbNotOpenError(this, db, Q_FUNC_INFO);
            return;
        }

        QString sql =
            "WITH last_edit AS ("
            "    SELECT application_id, MAX(dt) AS last_edit_dt "
            "    FROM application_views_audit "
            "    WHERE action = 'EDITED' "
            "    GROUP BY application_id"
            ") "
            "SELECT a.id AS application_id, "
            "       a.category_id, "
            "       a.application_name, "
            "       COALESCE(le.last_edit_dt, 0) AS last_edit_dt "
            "FROM application a "
            "LEFT JOIN last_edit le ON le.application_id = a.id "
            "WHERE COALESCE(le.last_edit_dt, 0) <= :cutoff;";

        QSqlQuery query(db);
        query.prepare(sql);
        query.bindValue(":cutoff", cutoffTs);

        if (!query.exec()) {
            showQueryError(this,query,Q_FUNC_INFO);
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id         = query.value("application_id").toInt();
            r.categoryId = query.value("category_id").toInt();
            r.appName    = DataObfuscator::deobfuscate(query.value("application_name").toString(), this->appKey);
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);
            r.description = query.value("last_edit_dt").toString();
            results.append(r);
        }
    }

    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(
            this,
            tr("Passwords Not Updated"),
            tr("No passwords were found that have not been updated since the selected date.")
        );
        return;
    }

    // -------------------------
    // Build MODELLESS dialog UI
    // -------------------------
    QDialog *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Passwords Not Updated Since %1")
                        .arg(QLocale().toString(cutoff, QLocale::ShortFormat)));

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QTableWidget *table = new QTableWidget(results.size(), 3, dlg);
    table->setHorizontalHeaderLabels({ tr("Password"), tr("Category"), tr("Last Edited") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(22);

    // 🔥 Prevent Qt from auto‑closing the dialog on double‑click
    disconnect(table, &QTableWidget::itemActivated, nullptr, nullptr);

    for (int row = 0; row < results.size(); ++row) {

        QTableWidgetItem *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole, results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        qint64 ts = results[row].description.toLongLong();
        QString shortDate = (ts == 0)
            ? tr("Never")
            : QLocale().toString(QDateTime::fromSecsSinceEpoch(ts), QLocale::ShortFormat);

        QTableWidgetItem *editedItem = new QTableWidgetItem(shortDate);
        editedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 2, editedItem);
    }

    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSortingEnabled(false);
    layout->addWidget(table);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("Close", dlg);
    QPushButton *okBtn     = new QPushButton("Select", dlg);
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(okBtn);
    layout->addLayout(buttonLayout);

    // -------------------------
    // Modeless behavior wiring
    // -------------------------

    auto selectRow = [this, dlg, table]() {
        int row = table->currentRow();
        if (row >= 0) {
            QTableWidgetItem *item = table->item(row, 0);
            int appId      = item->data(Qt::UserRole).toInt();
            int categoryId = item->data(Qt::UserRole + 1).toInt();
            selectInTreeWidgets(categoryId, appId);
        }
    };

    connect(okBtn, &QPushButton::clicked, dlg, [selectRow, dlg]() {
        selectRow();
        dlg->close();
    });

    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::close);

    connect(table, &QTableWidget::itemDoubleClicked, dlg, [selectRow]() {
        selectRow();
        // ❗ Do NOT close the dialog — user will want it to stay open
    });

    // Center dialog
    QSize parentSize = this->size();
    int w = static_cast<int>(parentSize.width() * 0.75);
    int h = static_cast<int>(parentSize.height() * 0.75);
    QPoint parentPos = this->pos();
    int x = parentPos.x() + (parentSize.width() - w) / 2;
    int y = parentPos.y() + (parentSize.height() - h) / 2;
    dlg->setGeometry(x, y, w, h);


    // --- EXPLICIT WIDTH CALCULATION (same fix as searchRecent) ---
    layout->activate();

    int totalWidth = table->verticalHeader()->width();
    for (int c = 0; c < table->columnCount(); ++c)
        totalWidth += table->columnWidth(c);

    totalWidth += table->frameWidth() * 2;

    QMargins m = layout->contentsMargins();
    totalWidth += m.left() + m.right();

    totalWidth += 16; // padding

    dlg->resize(totalWidth, dlg->sizeHint().height());
    // --- END WIDTH FIX ---

    dlg->show();   // MODELLESS
}

void MainWindow::openCategoryFromCurrent(QTreeWidgetItem* current, QTreeWidgetItem*)
{
    if (!current)
        return;

    // Call your existing slot with a default column (usually 0)
    openCategory(current, 0);
}


// void MainWindow::decryptWithGpg(
//         const QByteArray &encrypted,
//         std::function<void(const QByteArray&)> onSuccess,
//         std::function<void(const QString&)> onMissingKey,
//         std::function<void(const QString&)> onFailure)
// {
//     QProcess *gpg = new QProcess(this);
//     gpg->setProcessChannelMode(QProcess::SeparateChannels);

//     // Track whether we've shown the missing-key dialog
//     bool *noKeyShown = new bool(false);

//     // Feed data when ready
//     connect(gpg, &QProcess::started, this, [gpg, encrypted]() {
//         gpg->write(encrypted);
//         gpg->closeWriteChannel();
//     });

//     // Handle stdout (successful decrypt)
//     connect(gpg, &QProcess::readyReadStandardOutput, this, [gpg, onSuccess]() {
//         QByteArray out = gpg->readAllStandardOutput();
//         if (!out.isEmpty()) {
//             onSuccess(out);
//         }
//     });

//     // Handle stderr
//     connect(gpg, &QProcess::readyReadStandardError, this,
//             [this, gpg, noKeyShown, onMissingKey, onFailure]() {

//         QByteArray err = gpg->readAllStandardError();
//         if (err.isEmpty()) return;

//         QString errStr = QString::fromUtf8(err);

//         // NEW: show stderr in the status bar (restores original behavior)
//         ui->statusbar->showMessage(errStr);

//         qDebug().noquote() << "GPG stderr:" << errStr;

//         if (errStr.contains("no secret key", Qt::CaseInsensitive)) {
//             if (!*noKeyShown) {
//                 *noKeyShown = true;
//                 onMissingKey(errStr);
//             }
//         } else {
//             onFailure(errStr);
//         }
//     });

//     // Cleanup
//     connect(gpg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
//             this, [gpg, noKeyShown](int, QProcess::ExitStatus) {
//         delete noKeyShown;
//         gpg->deleteLater();
//     });

//     gpg->start("gpg", QStringList() << "--decrypt");
// }

void MainWindow::decryptWithGpg(
        const QByteArray &encrypted,
        std::function<void(const QByteArray&)> onSuccess,
        std::function<void(const QString&)> onMissingKey,
        std::function<void(const QString&)> onFailure)
{
    QProcess *gpg = new QProcess(this);
    gpg->setProcessChannelMode(QProcess::SeparateChannels);

    // Collect status lines and human-readable errors
    QStringList *statusLines = new QStringList();
    QStringList *humanErrors = new QStringList();

    connect(gpg, &QProcess::started, this, [gpg, encrypted]() {
        gpg->write(encrypted);
        gpg->closeWriteChannel();
    });

    connect(gpg, &QProcess::readyReadStandardOutput, this,
            [gpg, onSuccess]() {
        QByteArray out = gpg->readAllStandardOutput();
        if (!out.isEmpty())
            onSuccess(out);
    });

    connect(gpg, &QProcess::readyReadStandardError, this,
            [this, gpg, statusLines, humanErrors]() {

        QByteArray err = gpg->readAllStandardError();
        if (err.isEmpty())
            return;

        QString errStr = QString::fromUtf8(err);

        ui->statusbar->showMessage(errStr);
        qDebug().noquote() << "GPG stderr:" << errStr;

        const QStringList lines = errStr.split('\n', Qt::SkipEmptyParts);

        for (const QString &raw : lines) {
            QString line = raw.trimmed();
            if (line.startsWith("[GNUPG:]")) {
                statusLines->append(line);
            } else {
                humanErrors->append(line);
            }
        }
    });

    connect(gpg,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [gpg, statusLines, humanErrors,
             onMissingKey, onFailure](int exitCode, QProcess::ExitStatus status) {

        bool sawNoSecKey = false;
        int pkErrorCode = 0;

        // Parse collected status lines
        for (const QString &line : *statusLines) {
            if (line.contains("NO_SECKEY"))
                sawNoSecKey = true;

            if (line.contains("ERROR pkdecrypt_failed")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                bool ok = false;
                int code = parts.last().toInt(&ok);
                if (ok)
                    pkErrorCode = code;
            }
        }

        // Case 1: True missing secret key
        if (sawNoSecKey && pkErrorCode == 33554449) {
            onMissingKey("The secret key is not present.");
        }
        // Case 2: User cancelled
        else if (sawNoSecKey && pkErrorCode == 83886179) {
            // User cancelled — must call onFailure so cursor restores
            onFailure("Operation cancelled");
        }

        // Case 3: Other failure
        else if (status != QProcess::NormalExit || exitCode != 0) {
            if (!humanErrors->isEmpty())
                onFailure(humanErrors->join("\n"));
            else
                onFailure("GPG failed");
        }

        delete statusLines;
        delete humanErrors;
        gpg->deleteLater();
    });

    gpg->start("gpg", QStringList()
               << "--status-fd" << "2"
               << "--decrypt");
}
