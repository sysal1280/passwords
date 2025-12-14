#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "watermarkedtreewidget.h"
#include "droplabel.h"
#include "passwordDialog.h"
#include "settings.h"
#include "gpgCheck.h"
#include "preferencesdialog.h"
#include "DataObfuscator.h"
#include "SystemInfoDialog.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/qsqlquerymodel.h>
#include <QtSql/QSqlError>
#include <QProcess>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QInputDialog>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QClipboard>
#include <QTextEdit>
#include <QJsonDocument>
#include <QTableWidget>
#include <QListWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QByteArray>
#include <QLocale>
#include <QSysInfo>
#include <QActionGroup>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMap>
#include "newpassworddialog.h"
#include <QCloseEvent>
#include <QSpinBox>
#include <QShortcut>
#include <QSplitter>
#include <QResource>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDesktopServices>
#include <QUrl>

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
    layout->addWidget(ui->lineEdit);
    layout->addWidget(vSplitter);
    setCentralWidget(central);

    // Restore saved window and splitter states
    Settings::restoreMainWindowState(this);
    Settings::restoreSplitterState(vSplitter, "vsplit");
    Settings::restoreSplitterState(hSplitter, "hsplit");

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
        { ui->actionClose,                 ":/menus/glyphs/exit_to_app_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" },
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
        { ui->actionOnline_Documentation,  ":/menus/glyphs/help_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg" }
    };

    for (auto it = actionIcons.begin(); it != actionIcons.end(); ++it) {
        it.key()->setIcon(QIcon(it.value()));
    }

    //properties
    ui->actionAbout->setMenuRole(QAction::AboutQtRole);
    ui->actionAbout_Qt->setMenuRole(QAction::AboutQtRole);
    ui->actionClose->setMenuRole(QAction::QuitRole);
    ui->actionPreferences->setMenuRole(QAction::PreferencesRole);


    // OTP countdown widgets (progress bar + label)
    countdownProgress = new QProgressBar(this);
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
    connect(autoCloseTimer, &QTimer::timeout,
            this, &MainWindow::clearScrollArea);

    // Action triggers
    connect(ui->actionOpen_Password, &QAction::triggered,
            this, [this]() {
                if (auto item = ui->treeWidget_2->currentItem()) {
                    on_treeWidget_2_itemActivated(item, 0);
                }
            });

    // Line edit search
    connect(ui->lineEdit, &QLineEdit::returnPressed,
            this, [this]() {
                search(ui->lineEdit->text().trimmed());
            });

    // Tree widget custom signals
    connect(ui->treeWidget, &WatermarkedTreeWidget::itemDropped,
            this, &MainWindow::moveCategory);

    // Menu signals
    connect(ui->menuBookmarks, &QMenu::aboutToShow,
            this, &MainWindow::populateBookmarksMenu);

    connect(ui->actionOnline_Documentation, &QAction::triggered,
            this, [this]() {
                launchHelperProcess("");
            });

    connect(ui->actionDonate, &QAction::triggered,
            this, [this]() {
                launchHelperProcess("donate");
            });

    connect(ui->actionSystem_Information, &QAction::triggered,
            this, [this]() {
                SystemInfoDialog dlg(this);  // pass parent if needed
                dlg.exec();
            });

    connect(ui->actionRename, &QAction::triggered,
            this, &MainWindow::renameCategory);

    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::showAboutDlg);

    connect(ui->actionClose, &QAction::triggered,
            this, &MainWindow::close);


    connect(ui->treeWidget,
            &QTreeWidget::itemActivated,
            this,
            &MainWindow::openCategory);

    connect(ui->actionBookmark, &QAction::toggled,
            this, [this](bool checked) {
                setBookmark(checked);
            });


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

    connect(ui->actionPreferences, &QAction::triggered,
            this, [this]() {

                auto *pd = new PreferencesDialog(this);
                pd->setWindowTitle(ui->actionPreferences->text());
                pd->adjustSize();
                pd->setFixedSize(pd->size());
                pd->setWindowFlags(pd->windowFlags() & ~Qt::WindowMaximizeButtonHint);
                pd->exec();
            });

    connect(ui->actionRecent, &QAction::triggered,
            this, &MainWindow::searchRecent);

    connect(ui->actionPopular, &QAction::triggered,
            this, &MainWindow::searchPopular);



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
                        menu.addAction(ui->actionNew_Category);
                        menu.addAction(ui->actionNew_Password);
                        menu.addAction(ui->actionRename);
                        menu.addAction(ui->actionDelete_Category);
                        menu.addAction(ui->actionImport);
                        menu.addAction(ui->actionRefresh_Categories);
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
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (Settings::getCloseHelpServer()) {
        if (helperProcess) {
            // Disconnect error handler so no spurious popup
            disconnect(helperProcess, &QProcess::errorOccurred, nullptr, nullptr);
            if (helperProcess->state() != QProcess::NotRunning) {
                helperProcess->terminate();
                if (!helperProcess->waitForFinished(2000)) {
                    helperProcess->kill();
                }
                qInfo() << "Closed help server.";
            }
        } else
        {
            qInfo() << "Could not find a help server to close.";
        }
    }

    if (Settings::getAskClose()) {
        auto reply = QMessageBox::question(this,
                                           tr("Confirm Exit"),
                                           tr("Do you really want to quit?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    if (!qApp->property("skipSave").toBool()) {
        Settings::saveMainWindowState(this);
        Settings::saveSplitterState(vSplitter, "vsplit");
        Settings::saveSplitterState(hSplitter, "hsplit");
    }

    // Update last used file
    Settings::setLastUsedFile(qApp->property("dbFile").toString());

    event->accept();
}

void MainWindow::setActionIcon(QAction *action, const QString &iconPath) {
    action->setIcon(QIcon(iconPath));
}


void MainWindow::loadCategories()
{
    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            qCritical() << "Failed to open database:" << db.lastError().text();
            QSqlDatabase::removeDatabase(connectionName);
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
                qDebug() << "Query failed:" << query.lastError().text();
                return;
            }

            while (query.next()) {
                int id = query.value(0).toInt();
                int parentId = query.value(1).isNull() ? 0 : query.value(1).toInt();
                QString text = DataObfuscator::deobfuscate(query.value(2).toString(), appKey);

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
                qWarning() << "Orphan category id" << id << "parent" << parentId << "not found; leaving as top-level.";
                continue;
            }

            if (parentId == id) {
                qWarning() << "Self-parent detected for id" << id << "; skipping re-parent.";
                continue;
            }

            if (isAncestorOf(item, parent)) {
                qWarning() << "Cycle detected while linking id" << id << "-> parent" << parentId << "; skipping.";
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


// void MainWindow::loadCategories()
// {
//     const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

//     {
//         // Create and open a fresh connection in a local scope
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
//         db.setDatabaseName(qApp->property("dbFile").toString());
//         if (!db.open()) {
//             qCritical() << "Failed to open database:" << db.lastError().text();
//             QSqlDatabase::removeDatabase(connectionName);
//             return;
//         }

//         ui->treeWidget->blockSignals(true);
//         ui->treeWidget->clear();

//         QMap<int, QTreeWidgetItem*> itemMap;

//         {
//             QSqlQuery query(db);
//             if (!query.exec("SELECT id, parent_id, text FROM categories ORDER BY id")) {
//                 qDebug() << "Query failed:" << query.lastError().text();
//                 return; // db will be closed when it goes out of scope
//             }

//             // First pass: create items
//             while (query.next()) {
//                 int id = query.value(0).toInt();
//                 int parentId = query.value(1).toInt();
//                 QString text = DataObfuscator::deobfuscate(query.value(2).toString(), appKey);

//                 QTreeWidgetItem* item = new QTreeWidgetItem();
//                 item->setText(0, text);
//                 item->setData(0, Qt::UserRole, id);
//                 item->setIcon(0, closedIcon);
//                 itemMap[id] = item;
//                 if (parentId == 0) {
//                     ui->treeWidget->addTopLevelItem(item);
//                 }
//             }

//             // Second pass: link children
//             if (query.first()) {
//                 do {
//                     int id = query.value(0).toInt();
//                     int parentId = query.value(1).toInt();
//                     if (parentId != 0 && itemMap.contains(parentId)) {
//                         itemMap[parentId]->addChild(itemMap[id]);
//                     }
//                 } while (query.next());
//             }
//         } // query destroyed here

//         ui->treeWidget->blockSignals(false);

//         // Reconnect expand/collapse handlers
//         QObject::disconnect(ui->treeWidget, &QTreeWidget::itemExpanded, nullptr, nullptr);
//         QObject::disconnect(ui->treeWidget, &QTreeWidget::itemCollapsed, nullptr, nullptr);

//         connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, [=](QTreeWidgetItem *item){
//             if (item->childCount() > 0) {
//                 item->setIcon(0, QIcon(":/menus/glyphs/folder_open_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
//             }
//         });
//         connect(ui->treeWidget, &QTreeWidget::itemCollapsed, this, [=](QTreeWidgetItem *item){
//             if (item->childCount() > 0) {
//                 item->setIcon(0, QIcon(":/menus/glyphs/folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
//             }
//         });

//         // Select first item
//         if (ui->treeWidget->topLevelItemCount() > 0) {
//             QTreeWidgetItem *firstItem = ui->treeWidget->topLevelItem(0);
//             ui->treeWidget->setCurrentItem(firstItem);
//             firstItem->setSelected(true);
//             on_treeWidget_itemActivated(firstItem, 0);
//         }

//         db.close();
//     } // db destroyed here

//     // Now it’s safe to remove the connection
//     QSqlDatabase::removeDatabase(connectionName);
// }






void MainWindow::saveTreeToDb(QTreeWidget* tree, QSqlDatabase& db) {
    //QSqlQuery clearQuery(db);

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        saveItemToDb(item, db, QVariant()); // No parent for top-level
    }
}

void MainWindow::saveItemToDb(QTreeWidgetItem* item, QSqlDatabase& db, const QVariant& parentId) {
    QString itemText = item->text(0);
    int normalizedParentId = parentId.isNull() ? 0 : parentId.toInt();

    qDebug() << "Attempting to save item:" << itemText << "with parent_id:" << normalizedParentId;

    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO categories (parent_id, text) VALUES (?, ?)");
    query.addBindValue(normalizedParentId);
    query.addBindValue(DataObfuscator::obfuscate(itemText,appKey));

    if (!query.exec()) {
        qDebug() << "Insert failed:" << query.lastError().text();
    } else {
        qDebug() << "Insert attempted (may be ignored if duplicate).";
    }

    // Retrieve ID whether inserted or already existing
    QVariant id;
    QSqlQuery getId(db);
    getId.prepare("SELECT id FROM categories WHERE parent_id = ? AND text = ?");
    getId.addBindValue(normalizedParentId);
    getId.addBindValue(DataObfuscator::obfuscate(itemText,appKey));

    if (!getId.exec()) {
        qDebug() << "ID lookup failed:" << getId.lastError().text();
    } else if (getId.next()) {
        id = getId.value(0);
        qDebug() << "Retrieved ID for item:" << itemText << "->" << id.toInt();
    } else {
        qDebug() << "No ID found for item:" << itemText << "with parent_id:" << normalizedParentId;
    }

    // Recurse into children
    for (int i = 0; i < item->childCount(); ++i) {
        qDebug() << "Saving child" << i << "of item:" << itemText;
        saveItemToDb(item->child(i), db, id);
    }
}



MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showAboutDlg()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("About %1").arg(QCoreApplication::applicationName()));
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);

    // --- Icon ---
    QLabel *iconLabel = new QLabel(&dlg);
    QPixmap pix(":/password.png");
    iconLabel->setPixmap(pix.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(iconLabel);

    // Apply an opacity effect
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(iconLabel);
    iconLabel->setGraphicsEffect(opacityEffect);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(opacityEffect, "opacity");
    fadeIn->setDuration(1000);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    // --- Application name ---
    QLabel *nameLabel = new QLabel(QCoreApplication::applicationName(), &dlg);
    QFont nameFont;
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    nameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(nameLabel);

    // --- Version ---
    QLabel *versionLabel = new QLabel(QApplication::applicationVersion(), &dlg);
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);

    // --- Website link (URL exempt from translation) ---
    QLabel *websiteLabel = new QLabel(
        tr("Website: %1").arg(QStringLiteral("<a href=\"https://github.com/sysal1280/passwords\">https://github.com/sysal1280/passwords</a>")),
        &dlg);
    websiteLabel->setTextFormat(Qt::RichText);
    websiteLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    websiteLabel->setOpenExternalLinks(true);
    websiteLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(websiteLabel);

    // --- Copyright ---
    QFont smallPrintFont;
    smallPrintFont.setPointSize(8);
    smallPrintFont.setBold(false);

    QLabel *copyrightLabel = new QLabel(
        tr("Copyright © 2026 %1.<br/>").arg(QStringLiteral("Adam Lanzafame")),
        &dlg);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setFont(smallPrintFont);
    copyrightLabel->setTextFormat(Qt::RichText);
    copyrightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    // --- Word Lists ---
    QLabel *wordListLabel = new QLabel(
        tr("Wordlists from Orchard Street Wordlists by %1.")
            .arg(QStringLiteral("<a href=\"https://www.samschlinkert.com/\">Sam Schlinkert</a>")),
        &dlg);
    wordListLabel->setAlignment(Qt::AlignCenter);
    wordListLabel->setFont(smallPrintFont);
    wordListLabel->setTextFormat(Qt::RichText);
    wordListLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    wordListLabel->setOpenExternalLinks(true);

    // --- Material Symbols ---
    QLabel *materialSymbolsLabel = new QLabel(
        tr("Material Symbols from Google Fonts<br/>"
           "Licensed under the %1, Version 2.0.")
            .arg(QStringLiteral("<a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License</a>")),
        &dlg);
    materialSymbolsLabel->setAlignment(Qt::AlignCenter);
    materialSymbolsLabel->setFont(smallPrintFont);
    materialSymbolsLabel->setTextFormat(Qt::RichText);
    materialSymbolsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    materialSymbolsLabel->setOpenExternalLinks(true);

    // --- Password icon credit ---
    QLabel *iconLabelText = new QLabel(
        tr("Passwords icon created by %1.")
            .arg(QStringLiteral("<a href=\"https://www.flaticon.com/free-icons/password\">Iconic Panda - Flaticon</a>")),
        &dlg);
    iconLabelText->setAlignment(Qt::AlignCenter);
    iconLabelText->setFont(smallPrintFont);
    iconLabelText->setTextFormat(Qt::RichText);
    iconLabelText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    iconLabelText->setOpenExternalLinks(true);

    // --- License disclaimer ---
    QLabel *licenseLabel = new QLabel(
        tr("This program comes with absolutely no warranty.<br>"
           "See the %1 or later for details.")
            .arg(QStringLiteral("<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU General Public License, version 3</a>")),
        &dlg);
    licenseLabel->setWordWrap(true);
    licenseLabel->setAlignment(Qt::AlignCenter);
    licenseLabel->setTextFormat(Qt::RichText);
    licenseLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    licenseLabel->setOpenExternalLinks(true);
    licenseLabel->setFont(smallPrintFont);

    // --- GPLv3 logo ---
    QLabel *gplLogoLabel = new QLabel(&dlg);
    QPixmap gplLogo(":/pngs/gplv3-with-text-136x68.png");
    gplLogoLabel->setPixmap(gplLogo.scaledToWidth(136, Qt::SmoothTransformation));
    gplLogoLabel->setAlignment(Qt::AlignCenter);

    // --- Credits layout ---
    QVBoxLayout *creditsLayout = new QVBoxLayout;
    creditsLayout->setSpacing(6);
    creditsLayout->setContentsMargins(0,0,0,0);

    QFrame *line = new QFrame(&dlg);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    creditsLayout->addWidget(copyrightLabel);
    creditsLayout->addWidget(wordListLabel);
    creditsLayout->addWidget(materialSymbolsLabel);
    creditsLayout->addWidget(iconLabelText);
    creditsLayout->addSpacing(4);
    creditsLayout->addWidget(line);
    creditsLayout->addSpacing(4);
    creditsLayout->addWidget(licenseLabel);
    creditsLayout->addWidget(gplLogoLabel);

    mainLayout->addLayout(creditsLayout);

     dlg.resize(400, 440);
    dlg.setFixedSize(dlg.size());
    dlg.exec();
}


void MainWindow::on_actionNew_Password_triggered()
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

        qDebug().noquote() << "Running command:" << "gpg" << args.join(' ');

        QProcess process;
        process.start("gpg", args);
        if (!process.waitForStarted()) {
            qDebug() << "Failed to start GPG process.";
            QMessageBox::critical(this,ui->actionNew_Password->text(),tr("Failed to start GPG process."));
            return;
        }

        QByteArray jsonData = dlg.toJson();
        process.write(jsonData);
        process.waitForBytesWritten();
        process.closeWriteChannel();

        if (!process.waitForFinished()) {
            QMessageBox::critical(this,ui->actionNew_Password->text(),tr("GPG process did not finish correctly."));
            qCritical() << "GPG process did not finish correctly.";
            return;
        }

        QByteArray errors = process.readAllStandardError();
        if (!errors.isEmpty()) {
            qCritical().noquote() << "GPG Errors:\n" << QString::fromUtf8(errors);
            QMessageBox::critical(this,ui->actionNew_Password->text(),QString::fromUtf8(errors));
            return;
        }

        QFile outFile(tempFile);
        if (!outFile.exists() || !outFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Encrypted file not found or failed to open:" << tempFile;
            QFile::remove(tempFile);
            return;
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","sqlNewPassword");

            int categoryId = ui->treeWidget->currentItem()->data(0,Qt::UserRole).toInt();
            QString publicAppName = dlg.PublicAppName;
            QByteArray data = outFile.readAll();

            db.setDatabaseName(qApp->property("dbFile").toString());
            if (!db.open()) {
                qDebug() << "Failed to open database:" << db.lastError().text();
                return;
            }

            // --- Transaction start ---
            if (!db.transaction()) {
                qDebug() << "Failed to start transaction:" << db.lastError().text();
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
                qDebug() << "Insert failed:" << query.lastError().text();
                success = false;
            }

            int appId = query.lastInsertId().toInt();
            qDebug() << "last insert id" << appId;

            // Insert CREATED audit row
            if (success) {
                QSqlQuery audit(db);
                audit.prepare(R"(
                    INSERT INTO application_views_audit(application_id, dt, user, host, action, audit_time)
                    VALUES (:id, :dt, :user, :host, 'CREATED', strftime('%s','now')))");

                audit.bindValue(":id", appId);
                audit.bindValue(":dt", QDateTime::currentSecsSinceEpoch());
                audit.bindValue(":user", userName);
                audit.bindValue(":host", QSysInfo::machineHostName());

                if (!audit.exec()) {
                    qDebug() << "Audit insert failed:" << audit.lastError().text();
                    success = false;
                }
            }

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
                        qDebug() << "Token insert failed:" << insertToken.lastError().text();
                        success = false;
                        break;
                    }
                }
            }


            // Commit or rollback
            if (success) {
                if (!db.commit()) {
                    qDebug() << "Commit failed:" << db.lastError().text();
                    db.rollback();
                } else {
                    qDebug() << "Transaction committed successfully.";
                    if (!ui->treeWidget->selectedItems().isEmpty()) {
                        openCategory(ui->treeWidget->selectedItems().first(), 0);
                    }
                }
            } else {
                db.rollback();
                qDebug() << "Transaction rolled back due to errors.";
            }

            outFile.close();
            outFile.remove();

        }
        QSqlDatabase::removeDatabase("sqlNewPassword");
    }
}


QList<KeyEntry> MainWindow::fetchKeys() const
{
    QString connName = QUuid::createUuid().toString();
    QList<KeyEntry> keys;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open())
        {
            QSqlQuery query(db);
            query.setForwardOnly(true);
            query.exec("SELECT id, label, key FROM keys");
            while (query.next()) {
                KeyEntry entry;
                entry.id    = query.value(0).toInt();
                entry.label = query.value(1).toString();
                entry.key   = query.value(2).toString();
                keys.append(entry);
            }
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return keys;
}

void MainWindow::on_actionNew_Category_triggered()
{
    createCategory();
    return;
    QString text = QInputDialog::getText(this, tr("Category"), tr("Create a new category:")).trimmed();
    if (text.isEmpty()) {

        return;
    }

    static const QRegularExpression categoryNameRe("^[\\w\\-\\s]{1,64}$");
    if (!categoryNameRe.match(text).hasMatch()) {
        QMessageBox::warning(this,
                             tr("Invalid Name"),
                             tr("Category name contains invalid characters or is too long."));
        return;
    }


    QTreeWidgetItem* parentItem = ui->treeWidget->currentItem();
    QVariant parentId = QVariant(QMetaType(QMetaType::Int)); // NULL by default
    if (parentItem) {
        parentId = parentItem->data(0, Qt::UserRole); // parent’s DB id
    }

    // Duplicate check in UI
    QTreeWidgetItem* scope = parentItem ? parentItem : ui->treeWidget->invisibleRootItem();
    for (int i = 0; i < scope->childCount(); ++i) {
        if (scope->child(i)->text(0) == text) {
            QMessageBox::warning(this, tr("Duplicate"), tr("A category with this name already exists here."));
            return;
        }
    }

    // DB connection
    QString connName = QUuid::createUuid().toString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(qApp->property("dbFile").toString());

    if (!db.open()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to open database:\n%1").arg(db.lastError().text()));
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO categories (parent_id, text) VALUES (:parent_id, :text)");
    query.bindValue(":parent_id", parentId); // NULL if top-level
    query.bindValue(":text", DataObfuscator::obfuscate(text,this->appKey));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Insert failed:\n%1").arg(query.lastError().text()));
        db.close();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    int newId = query.lastInsertId().toInt();

    // Create tree item with its OWN id
    QTreeWidgetItem* newItem = new QTreeWidgetItem();
    newItem->setText(0, text);
    newItem->setIcon(0, QIcon(":/menus/glyphs/folder_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    newItem->setData(0, Qt::UserRole, newId); // <-- store record’s id here

    if (parentItem) {
        parentItem->addChild(newItem);
        parentItem->setExpanded(true);
    } else {
        ui->treeWidget->addTopLevelItem(newItem);
    }

    db.close();
    QSqlDatabase::removeDatabase(connName);
}

void MainWindow::openCategory(QTreeWidgetItem *item, int column) {
    QString connName = QUuid::createUuid().toString();
    clearScrollArea();

    qDebug() << item->data(0,Qt::UserRole).toInt() << "userrole id";

    // Block signals and clear the second tree widget
    ui->treeWidget_2->blockSignals(true);
    ui->treeWidget_2->clear();

    // Create the database connection
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            qDebug() << "Failed to open database:" << db.lastError().text();
            ui->treeWidget_2->blockSignals(false); // Unblock signals on error
            return;
        }

        // Prepare and execute the query
        QSqlQuery query(db);
        query.prepare(QString("SELECT * FROM application WHERE category_id = :id ORDER BY id"));
        query.bindValue(":id", item->data(0,Qt::UserRole).toInt());

        if (!query.exec()) {
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

    // Reset openedCredentialID
    openedCredentialID = -1;
}


// void MainWindow::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column) {
//     QString connName = QUuid::createUuid().toString();
//     clearScrollArea();

//     qDebug() << item->data(0,Qt::UserRole).toInt() << "userrole id";

//     // Block signals and clear the second tree widget
//     ui->treeWidget_2->blockSignals(true);
//     ui->treeWidget_2->clear();

//     // Create the database connection
//     {
//         QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
//         db.setDatabaseName(qApp->property("dbFile").toString());

//         if (!db.open()) {
//             qDebug() << "Failed to open database:" << db.lastError().text();
//             ui->treeWidget_2->blockSignals(false); // Unblock signals on error
//             return;
//         }

//         // Prepare and execute the query
//         QSqlQuery query(db);
//         query.prepare(QString("SELECT * FROM application WHERE category_id = :id ORDER BY id"));
//         query.bindValue(":id", item->data(0,Qt::UserRole).toInt());

//         if (!query.exec()) {
//             db.close();
//             QSqlDatabase::removeDatabase(connName);
//             ui->treeWidget_2->blockSignals(false);
//             return;
//         }

//         // Process each row of the query result and create tree items
//         while (query.next()) {
//             QTreeWidgetItem* newItem = nullptr;
//              newItem = makeItemFromApplication(query);
//             if (newItem) {
//                 ui->treeWidget_2->addTopLevelItem(newItem);
//             }
//         }

//         // Clean up
//         db.close();
//     }
//     QSqlDatabase::removeDatabase(connName);

//     // Unblock signals after processing is complete
//     ui->treeWidget_2->blockSignals(false);

//     // Reset openedCredentialID
//     openedCredentialID = -1;
// }




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
            this, [this](QTreeWidgetItem *item){
                on_treeWidget_2_itemActivated(item, 0); // supply a column yourself
            });

    openedCredentialID = -1;
}




void MainWindow::on_treeWidget_2_itemActivated(QTreeWidgetItem *item, int column)
{
    int parentId = item->data(0, Qt::UserRole).toInt();

    if (openedCredentialID == parentId)
    {
        clearScrollArea();
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    if (Settings::getKillGpgAgent()) {
        killGpgAgent();
    }

    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // --- Step 0: Map mode to table/column names ---
    QString viewTable, idColumn, credTable;

    viewTable      = "application_views";
    idColumn       = "application_id";
    credTable      = "application";

    // --- Step 1: Retrieve encrypted data ---
    ui->statusbar->showMessage("Reading database..");
    QApplication::processEvents();

    QByteArray data;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare(QString("SELECT data FROM application WHERE id = :id"));
            query.bindValue(":id", parentId);
            if (query.exec() && query.first()) {
                data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
                if (showDebugMessages) {
                    QMessageBox::information(this,"Test",data);
                }
            } else {
                qDebug() << "Query failed:" << query.lastError().text();
            }
        } else {
            qDebug() << db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt the data (async, non-blocking) ---
    ui->statusbar->showMessage("Decrypting data..");
    QApplication::processEvents();

    QProcess* gpg = new QProcess(this);
    gpg->setProcessChannelMode(QProcess::SeparateChannels);

    connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
        gpg->write(data);
        data.fill(0);
        gpg->closeWriteChannel();
    });

    // Handle stdout (decrypted JSON)
    connect(gpg, &QProcess::readyReadStandardOutput, this, [this, gpg, item]() {
        QByteArray decrypted_data = gpg->readAllStandardOutput();
        if (!decrypted_data.isEmpty()) {
            ui->statusbar->clearMessage();

            if (showDebugMessages) {
                QMessageBox::information(this,"Decrypted",decrypted_data);
            }

            //parseJsonApplication(decrypted_data);
            populateFromJsonApplication(decrypted_data, ui);

            // if (mode == "application") {
            //     parseJsonApplication(decrypted_data);
            //     populateFromJsonApplication(decrypted_data, ui);
            // } else if (mode == "note") {
            //     parseJsonNote(decrypted_data);
            // } else if (mode == "file") {
            //     parseJsonFile(decrypted_data, ui->treeWidget_2->currentItem()->text(0));
            //     populateFromJsonFile(decrypted_data, ui);
            // } else if (mode == "credit") {
            //     parseJsonCredit(decrypted_data);
            //     populateFromJsonCredit(decrypted_data, ui);
            // }
            decrypted_data.fill(0);
        }
    });

    // Handle stderr
    connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg]() {
        QByteArray errors = gpg->readAllStandardError();
        if (!errors.isEmpty()) {
            ui->statusbar->showMessage(QString::fromUtf8(errors));
            qDebug().noquote() << "GPG stderr:" << errors;
        }
    });

    // --- Step 4: Handle completion ---
    connect(gpg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, gpg, parentId, connName, viewTable, idColumn](int exitCode, QProcess::ExitStatus status) {
                if (status == QProcess::NormalExit && exitCode == 0) {
                    openedCredentialID = parentId;

                    // Upsert into views table only after successful decryption
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
                                qDebug() << "ERROR:" << upsert.lastError().text();
                            }
                        }
                    }
                    QSqlDatabase::removeDatabase(connName);

                } else {
                    openedCredentialID = -1;
                }

                gpg->deleteLater();
                QApplication::restoreOverrideCursor();
                QApplication::processEvents();
            });

    gpg->start("gpg", QStringList() << "--decrypt");
}



void MainWindow::parseJson(const QByteArray &jsonData)
{
    // Parse JSON from QByteArray
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    // Root object
    QJsonObject root = doc.object();

    // Read simple fields
    QString url = root.value("url").toString();
    QString description = root.value("description").toString();

    qDebug() << "URL:" << url;
    qDebug() << "Description:" << description;

    // Read credentials array
    QJsonArray credentials = root.value("credentials").toArray();
    for (const QJsonValue &val : std::as_const(credentials)) {
        QJsonObject credObj = val.toObject();
        QString username = credObj.value("username").toString();
        QString password = credObj.value("password").toString();
        QString secretOptCode = credObj.value("secretOptCode").toString();

        qDebug() << "User:" << username
                 << "Password:" << password
                 << "Secret Code:" << secretOptCode;
    }
}

void MainWindow::populateFromJson(const QByteArray &jsonData, Ui::MainWindow *ui) {
    // Ensure scrollArea has a widget
    if (!ui->scrollArea->widget()) {
        QWidget *container = new QWidget;
        ui->scrollArea->setWidget(container);
        ui->scrollArea->setWidgetResizable(true);
    }

    setupAlignedTimer();

    // Get or create grid layout
    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui->scrollArea->widget()->layout());
    if (!gridLayout) {
        gridLayout = new QGridLayout(ui->scrollArea->widget());
        ui->scrollArea->widget()->setLayout(gridLayout);
    }

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

    // Add a full-width label in the next row for description
    // Add a full-width label in the next row for name/description
    QString name        = root.value("private_name").toString().trimmed();
    QString description = root.value("description").toString().trimmed();

    QString labelText;
    if (!name.isEmpty() && !description.isEmpty()) {
        labelText = name + ", " + description;
    } else if (!name.isEmpty()) {
        labelText = name;
    } else if (!description.isEmpty()) {
        labelText = description;
    } else {
        labelText = ""; // nothing to show
    }

    if (!labelText.isEmpty()) {
        QLabel* fullWidthLabel = new QLabel(labelText);
        fullWidthLabel->setStyleSheet("background-color: #0064b9; padding: 6px; color: white; font-weight: bold;");
        gridLayout->addWidget(fullWidthLabel, 0, 0, 1, -1); // span all columns
    }


    // Add a full-width label in the next row for description
    if (root.contains("url")) {
        QString url = root.value("url").toString();
        QLabel* fullWidthLabel_url = new QLabel("<a href=\"" + url + "\">" + url + "</a>");
        fullWidthLabel_url->setTextFormat(Qt::RichText);
        fullWidthLabel_url->setTextInteractionFlags(Qt::TextBrowserInteraction);
        fullWidthLabel_url->setOpenExternalLinks(true);
        fullWidthLabel_url->setStyleSheet("padding-bottom: 16px");

        gridLayout->addWidget(fullWidthLabel_url, 1, 0, 1, -1); // span all columns
    }

    int row = 2;
    for (const QJsonValue &val : std::as_const(credentials)) {
        QJsonObject credObj = val.toObject();
        QString username = credObj.value("username").toString();
        QString password = credObj.value("password").toString();
        QString otp      = credObj.value("secretOptCode").toString();
        int otpLength      = credObj.value("length").toInt(6);
        qDebug() << "length " << otpLength;

        // Username
        QLabel* usernameLabel = new QLabel("Username");
        QLineEdit* usernameEdit = new QLineEdit();
        usernameEdit->setText(username);
        usernameEdit->setContextMenuPolicy(Qt::ActionsContextMenu);
        usernameEdit->setReadOnly(true);

        // Create the action
        QAction *copyUsernameAction = new QAction(tr("Copy Username"), usernameEdit);
        copyUsernameAction->setStatusTip("Username ony remains in clipboard for 15 seconds");
        // Add it to the line edit’s context menu
        usernameEdit->addAction(copyUsernameAction);

        // Connect it to a slot or lambda
        connect(copyUsernameAction, &QAction::triggered, this, [usernameEdit]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(usernameEdit->text());

            // Clear clipboard after 15 seconds
            QTimer::singleShot(15000, qApp, [clipboard]() {
                clipboard->clear();
                clipboard->setText("");
                qDebug() << "clipboard cleared";
            });
        });

        // Password
        QLabel* passwordLabel = new QLabel("Password");
        QLineEdit* passwordEdit = new QLineEdit();
        passwordEdit->setText(password);
        passwordEdit->setContextMenuPolicy(Qt::ActionsContextMenu);
        passwordEdit->setEchoMode(QLineEdit::Password);
        passwordEdit->setReadOnly(true);

        QAction *showPasswordAction = new QAction(tr("Show Password"), passwordEdit);
        showPasswordAction->setCheckable(true);
        passwordEdit->addAction(showPasswordAction);

        connect(showPasswordAction, &QAction::toggled, this, [passwordEdit](bool checked) {
            passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        });


        // Create the action
        QAction *copyPasswordAction = new QAction(tr("Copy Password"), passwordEdit);
        copyPasswordAction->setStatusTip("Password ony remains in clipboard for 15 seconds");
        // Add it to the line edit’s context menu
        passwordEdit->addAction(copyPasswordAction);

        // Connect it to a slot or lambda
        connect(copyPasswordAction, &QAction::triggered, this, [passwordEdit]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(passwordEdit->text());

            // Clear clipboard after 15 seconds
            QTimer::singleShot(15000, qApp, [clipboard]() {
                clipboard->clear();
                clipboard->setText("");
                qDebug() << "clipboard cleared";
            });

        });

        // OTP Code (optional)
        QLabel* otpLabel = new QLabel("OTP Code");
        QLineEdit* otpEdit = new QLineEdit();
        otpEdit->setText(otp);
        otpEdit->setPlaceholderText(otp);
        otpEdit->setObjectName("otpEdit"+QString::number(row));
        otpEdit->setProperty("otpLength",otpLength);
        otpEdit->setReadOnly(true);

        // Add to grid: 6 widgets per row
        gridLayout->addWidget(usernameLabel, row, 0);
        gridLayout->addWidget(usernameEdit,  row, 1);
        gridLayout->addWidget(passwordLabel, row, 2);
        gridLayout->addWidget(passwordEdit,  row, 3);
        gridLayout->addWidget(otpLabel,      row, 4);
        gridLayout->addWidget(otpEdit,       row, 5);

        ++row;
    }


    // --- Notes section ---
    if (root.contains("notes")) {
        QJsonArray notesArray = root.value("notes").toArray();

        // Section header
        QLabel* notesHeader = new QLabel("Notes");
        notesHeader->setStyleSheet("background-color: lightgray; padding: 6px; margin-top: 16px;");
        gridLayout->addWidget(notesHeader, row, 0, 1, -1);
        row++;

        // Each note as a full-width label
        for (const QJsonValue &noteVal : std::as_const(notesArray)) {
            QJsonObject noteObj = noteVal.toObject();
            QString content = noteObj.value("content").toString();

            QLabel* noteLabel = new QLabel(content);
            noteLabel->setWordWrap(true); // allow multi-line wrapping
            noteLabel->setStyleSheet("padding: 4px;"); // optional styling
            gridLayout->addWidget(noteLabel, row, 0, 1, -1);
            row++;
        }
    }


    // Add a full-width label in the next row for Password/Object Details

        QLabel* fullWidthLabel = new QLabel("About this password");
        fullWidthLabel->setStyleSheet("background-color: lightgray; padding: 6px; margin-top: 16px;");
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
            QMessageBox::critical(this, this->windowTitle(),
                                  QString("Could not open %1\n%2.")
                                      .arg(qApp->property("dbFile").toString(), db.lastError().text()));
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
                        qDebug() << query.value(0).toString()
                        << query.value(1).toString()
                        << query.value(2).toString()
                        << query.value(3).toString()
                        << query.value(4).toString();


                        // Accessed count (formatted with thousands separators)
                        QLocale locale = QLocale::system();
                        QString accessedText = QString("Accessed: %1 times")
                                                   .arg(locale.toString(query.value(0).toLongLong()));
                        QLabel* accessedLabel = new QLabel(accessedText);
                        accessedLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
                        gridLayout->addWidget(accessedLabel, row, 0, 1, -1);
                        row++;

                        // Last accessed datetime (convert from Unix epoch)
                        qint64 epoch = query.value(1).toLongLong();
                        QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch);
                        QString lastAccessedText = QString("Last access: %1")
                                                       .arg(locale.toString(dt, QLocale::ShortFormat));
                        QLabel* lastAccessedLabel = new QLabel(lastAccessedText);
                        lastAccessedLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
                        gridLayout->addWidget(lastAccessedLabel, row, 0, 1, -1);
                        row++;

                        // Last access by (assuming column 4 holds the username)
                        QString lastAccessByText = QString("Last access by: %1 on %2")
                                                       .arg(query.value(2).toString(), query.value(3).toString());
                        QLabel* lastAccessByLabel = new QLabel(lastAccessByText);
                        lastAccessByLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
                        gridLayout->addWidget(lastAccessByLabel, row, 0, 1, -1);
                        row++;

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
        20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(verticalSpacer, row+1, 0, 1, -1);



    updateFields();


    /*
     * Setup any autoclose singlshot timer
     */
    int autoClose = Settings::getAutoCloseSeconds();
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
        qWarning() << "Failed to start gpgconf process";
        return false;
    }

    if (!process.waitForFinished(2500)) {
        qWarning() << "gpgconf process did not finish";
        return false;
    }

    QByteArray output = process.readAllStandardOutput();
    QByteArray errors = process.readAllStandardError();

    if (!output.isEmpty())
        qDebug() << "gpgconf output:" << output;
    if (!errors.isEmpty())
        qDebug() << "gpgconf errors:" << errors;

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
    qDebug() << "AA";
    QTime now = QTime::currentTime();
    int sec = now.second();

    int remaining = (sec < 30) ? (30 - sec) : (60 - sec);
    countdownLabel->setText(QString("%1").arg(remaining, 2, 10, QChar('0')));
    countdownProgress->setValue(remaining);

    countdownLabel->setVisible(true);
    countdownProgress->setVisible(true);

    updateFields();
}


void MainWindow::updateFields()
{
    qDebug() << "BB";

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
                    edit->setText(newValue);
                }
            } else {
                edit->setText("NA");
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

void MainWindow::on_actionShow_Password_toggled(bool arg1)
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action) return;

    // Find the QLineEdit this action is attached to
    QLineEdit *edit = nullptr;
    const QList<QObject*> associated = action->associatedObjects();
    for (QObject *obj : associated) {
        if ((edit = qobject_cast<QLineEdit*>(obj))) {
            break;
        }
    }
    if (!edit) return;

    edit->setEchoMode(arg1 ? QLineEdit::Normal : QLineEdit::Password);
}

void MainWindow::on_treeWidget_2_customContextMenuRequested(const QPoint &pos)
{
    auto selected = ui->treeWidget_2->selectedItems();
    if (selected.isEmpty()) return;

    QPoint globalPos = ui->treeWidget_2->viewport()->mapToGlobal(pos);
    QMenu menu;

    int selectedId = selected.first()->data(0,Qt::UserRole).toInt();
    qDebug() << openedCredentialID << selectedId;

    if (openedCredentialID == selectedId) {
        QAction *actionCloseCredential = new QAction("Close Password", &menu);
        actionCloseCredential->setIcon(QPixmap(":/menus/glyphs/lock_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
        menu.addAction(actionCloseCredential);
        connect(actionCloseCredential, &QAction::triggered,
                this, &MainWindow::clearScrollArea);
        menu.setDefaultAction(actionCloseCredential);
    } else {
        menu.addAction(ui->actionOpen_Password);
        menu.setDefaultAction(ui->actionOpen_Password);
    }

    menu.addAction(ui->actionEdit_Password);
    menu.addAction(ui->actionExport_Password);
    menu.addAction(ui->actionAdd_Search);
    menu.addAction(ui->actionDelete_Password);
     menu.addSeparator();
    menu.addAction(ui->actionBookmark);
    menu.addSeparator();
    menu.addAction(ui->actionAudit_Log);

    // --- Bookmark check ---
    QString connName = QUuid::createUuid().toString();
    {

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT 1 FROM favourite WHERE username = :username AND application_id = :id");
            query.bindValue(":username", this->userName);
            query.bindValue(":id", selectedId);

            bool isBookmarked = (query.exec() && query.next());
            ui->actionBookmark->setCheckable(true);
            ui->actionBookmark->setChecked(isBookmarked);
        }
        db.close();

    }
    QSqlDatabase::removeDatabase(connName);

    // ----------------------

    menu.exec(globalPos);
}


void MainWindow::on_actionShow_debug_messages_triggered(bool checked)
{
    showDebugMessages = checked;
    qDebug() << "showDebugMessages" << showDebugMessages;
}


QTreeWidgetItem* MainWindow::makeItemFromApplication(QSqlQuery& query) {
    int id       = query.value(0).toInt();
    //int parentId = query.value(1).toInt();
    QString name = DataObfuscator::deobfuscate(query.value(2).toString(),appKey);

    //qDebug() << Q_FUNC_INFO << id << parentId << name << url;

    auto* item = new QTreeWidgetItem();
    item->setText(0, name);
    item->setData(0, Qt::UserRole, id);
    item->setIcon(0,QPixmap(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    //item->setText(1, QString::number(id));
    return item;
}

QTreeWidgetItem* MainWindow::makeItemFromNote(QSqlQuery& query) {
    int id       = query.value(0).toInt();
    int parentId = query.value(1).toInt();
    QString title = query.value(2).toString();
    //QString body  = query.value(3).toString();

    auto* item = new QTreeWidgetItem();
    item->setText(0, title);
    item->setText(1, QString::number(id));
    item->setData(0, Qt::UserRole, parentId);
    return item;
}

QTreeWidgetItem* MainWindow::makeItemFromFile(QSqlQuery& query) {
    int id       = query.value(0).toInt();
    int parentId = query.value(1).toInt();
    QString title = query.value(2).toString();

    auto* item = new QTreeWidgetItem();
    item->setText(0, title);
    item->setText(1, QString::number(id));
    item->setData(0, Qt::UserRole, parentId);
    return item;
}

QTreeWidgetItem* MainWindow::makeItemFromCredit(QSqlQuery& query) {
    int id       = query.value(0).toInt();
    int parentId = query.value(1).toInt();
    QString title = query.value(2).toString();
    //QString body  = query.value(3).toString();

    auto* item = new QTreeWidgetItem();
    item->setText(0, title);
    item->setText(1, QString::number(id));
    item->setData(0, Qt::UserRole, parentId);
    return item;
}

void MainWindow::populateFromJsonApplication(const QByteArray &jsonData, Ui::MainWindow *ui) {
        parseJson(jsonData);
        populateFromJson(jsonData,ui);

        QApplication::restoreOverrideCursor();
        QApplication::processEvents();

        if (Settings::getKillGpgAgent()) {
            killGpgAgent();
        }
}


void MainWindow::parseJsonNote(const QByteArray &jsonData) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonObject obj = doc.object();

    QString title = obj.value("title").toString("Untitled");
    QString body  = obj.value("body").toString();

    qDebug() << "Note title:" << title;
        qDebug() << "Note body:" << body;


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

        // Clear existing widgets
        while (QLayoutItem* item = gridLayout->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        // Add title label
        if (obj.contains("title")) {
            QLabel* titleLabel = new QLabel(title);
            titleLabel->setStyleSheet("background-color: #0064b9; padding: 6px; color: white; font-weight: bold;");
            gridLayout->addWidget(titleLabel, 0, 0, 1, -1); // span all columns
        }

        // Add body label
        if (obj.contains("body")) {
            QLabel* bodyLabel = new QLabel(body);
            bodyLabel->setStyleSheet("padding: 10px;");
            bodyLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            gridLayout->addWidget(bodyLabel, 1, 0, 1, -1); // span all columns
        }

        // Add a vertical spacer to push everything up
        QSpacerItem *verticalSpacer = new QSpacerItem(
            20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayout->addItem(verticalSpacer, 2, 0, 1, -1);
}

void MainWindow::parseJsonFile(const QByteArray &jsonData, QString filename)
    {
        // Show a save file dialog with suggested filename
        QString selectedPath = QFileDialog::getSaveFileName(
            this,
            tr("Save Decrypted File"),
            filename,                  // suggested filename
            tr("All Files (*.*)")           // filter
            );

        // If user cancels, selectedPath will be empty
        if (selectedPath.isEmpty()) {
            return;
        }

        QFile outFile(selectedPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Could not open file for writing:\n%1").arg(outFile.errorString()));
            return;
        }

        // Write the decrypted contents
        outFile.write(jsonData);
        outFile.close();

        QMessageBox::information(this, tr("Success"),
                                 tr("File saved successfully:\n%1").arg(selectedPath));
    }

void MainWindow::on_actionKey_List_triggered()
{
    const QString dbFile = qApp->property("dbFile").toString();
    if (Q_UNLIKELY(dbFile.isEmpty())) {
        QMessageBox::warning(this, ui->actionKey_List->text(), "No database open.");
        return;
    }


    // Create dialog
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionKey_List->text());
    dlg->setWindowFlags(dlg->windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction label at the top
    QLabel *instruction = new QLabel(
        "Keys listed here are only linked, not stored, this application references your existing GPG keyring."
        "Your private keys remain under your control and are never copied into this database.\n\n"
        "If you lose access to a private key that was used to encrypt data,"
        "there is NO WAY to recover that data. Keep backups of your keys in a safe place.",
        dlg
        );
    instruction->setWordWrap(true);
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
    table->verticalHeader()->setDefaultSectionSize(20);
    // make header font non‑bold
    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(false);
    table->horizontalHeader()->setFont(headerFont);

    // Populate from DB
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetchkeys");
        db.setDatabaseName(dbFile);
        db.open();

        QSqlQuery query(db);
        if (query.exec("SELECT label, key FROM keys")) {
            int row = 0;
            while (query.next()) {
                table->insertRow(row);
                table->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
                table->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
                row++;
            }
        } else {
            qWarning() << "Failed to query keys:" << query.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase("fetchkeys");

    layout->addWidget(table);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *importBtn = new QPushButton("&Link Key", dlg);
    QPushButton *deleteBtn = new QPushButton("&Delete", dlg);   // new delete button
    QPushButton *helpBtn   = new QPushButton("&Help", dlg);
    QPushButton *closeBtn  = new QPushButton("&Close", dlg);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addWidget(deleteBtn);
    buttonLayout->addWidget(helpBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    // Hook up import
    connect(importBtn, &QPushButton::clicked, this, [=]() {
        bool ok;
        QString keyId = QInputDialog::getText(dlg, "Import Key", "Enter GPG Key ID:", QLineEdit::Normal, "", &ok);
        if (!ok || keyId.isEmpty()) return;

        // Duplicate Key check
        bool duplicate = false;
        for (int r = 0; r < table->rowCount(); ++r) {
            QTableWidgetItem *nameItem = table->item(r, 0);
            QTableWidgetItem *keyItem  = table->item(r, 1);
            if                 (keyItem  && keyItem->text()  == keyId) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            QMessageBox::warning(this,
                                 ui->actionKey_List->text(),
                                 tr("This key ID or label is already in the list."));
            return;
        }

        QString name = QInputDialog::getText(dlg, "Import Key", "Enter Name:", QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insertkeys");
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (db.open())
            {
            QSqlQuery insert(db);
            insert.prepare("INSERT INTO keys (label, key) VALUES (:label, :key)");
            insert.bindValue(":key", keyId);
            insert.bindValue(":label", name);
            if (!insert.exec()) {
                qWarning() << "Failed to insert key:" << insert.lastError().text();
                QMessageBox::critical(this,ui->actionKey_List->text(),insert.lastError().text());
                return;
            }
            }
        }
        QSqlDatabase::removeDatabase("insertkeys");

        // Update table dynamically
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(name));
        table->setItem(row, 1, new QTableWidgetItem(keyId));
    });

    // Hook up delete
    connect(deleteBtn, &QPushButton::clicked, this, [=]() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(dlg, "Delete Key", "Please select a key to delete.");
            return;
        }

        QString name  = table->item(row, 0)->text();
        QString keyId = table->item(row, 1)->text();

        if (QMessageBox::question(dlg, tr("Confirm Delete"),
                                  QString(tr("Delete key '%1' (%2)?")).arg(name, keyId))
            != QMessageBox::Yes) {
            return;
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "deletekeys");
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (db.open())
            {

            if (!Settings::verifyDeleteAllowed(db, this)) {
                QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
                return; // bail out early
            }

            QSqlQuery remove(db);
            remove.prepare("DELETE FROM keys WHERE key = :key");
            remove.bindValue(":key", keyId);
            if (!remove.exec()) {
                qWarning() << "Failed to delete key:" << remove.lastError().text();
                QMessageBox::critical(dlg, tr("Error"), tr("Failed to unlink key from database."));
                return;
            }
            }
        }
        QSqlDatabase::removeDatabase("deletekeys");

        table->removeRow(row); // remove from table view
    });

    table->resizeColumnsToContents();

    // After you add all widgets and set the layout:
    dlg->layout()->setSizeConstraint(QLayout::SetFixedSize);
    dlg->setFixedSize(dlg->sizeHint());

    // Hook up help
    connect(helpBtn, &QPushButton::clicked, this, [=]() {
        launchHelperProcess("keys");
    });

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->setLayout(layout);
    dlg->resize(400, 300);
    dlg->exec();
}


void MainWindow::on_actionAudit_Log_triggered()
{
    /*
     * Password Audit Log
     */

    // Create dialog
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionAudit_Log->text());

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction label
    QLabel *instruction = new QLabel(tr(
                                         "This view shows the password audit log.\n"
                                         "It records actions performed, by which user and host, and when.\n"
                                         "Note: application_id is used internally as a key and is not displayed."),
                                     dlg
                                     );
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
    table->verticalHeader()->setDefaultSectionSize(20);

    // Populate from DB
    int appId = ui->treeWidget_2->currentItem()->data(0, Qt::UserRole).toInt();
    QString connName = QUuid::createUuid().toString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(qApp->property("dbFile").toString());
    if (!db.open()) {
        qDebug() << "Failed to open DB:" << db.lastError().text();
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
            qDebug() << "Failed to query audit log:" << query.lastError().text();
        }
    }

    layout->addWidget(table);
    table->resizeColumnsToContents();

    // Buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *exportBtn = new QPushButton("&Export", dlg);
    QPushButton *closeBtn = new QPushButton("&Close", dlg);
    buttonLayout->addStretch();
    buttonLayout->addWidget(exportBtn);
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
                // Quote each field to protect spaces/commas
                fields << "\"" + text.replace("\"", "\"\"") + "\"";
            }
            out << fields.join(",") << "\n";
        }

        file.close();
        QMessageBox::information(dlg, "Export Complete", "Audit log exported successfully.");
    });

    // Clean up connection when dialog closes
    connect(dlg, &QDialog::finished, this, [connName](int){
        QSqlDatabase db = QSqlDatabase::database(connName);
        db.close();
        QSqlDatabase::removeDatabase(connName);
    });

    dlg->setLayout(layout);
    dlg->resize(700, 400);
    dlg->exec();
}



void MainWindow::on_actionRefresh_Categories_triggered()
{
    this->loadCategories();
}


void MainWindow::on_actionGenerate_Password_triggered()
{
    // Just delegate to the reusable dialog
    PasswordDialog::showPasswordGenerator(
        this,
        ui->actionGenerate_Password->text(),
        {} // we ignore this parameter; the function loads the wordlist itself
        );
}

void MainWindow::on_actionEncrypt_message_triggered()
{
    /*
     * Encrypt Message Dialog (GPG symmetric, ASCII-armored)
     * Uses --pinentry-mode loopback to pass the dialog-entered password to gpg.
     */

    // Dialog and layout
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionEncrypt_message->text());
    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction
    QLabel *instruction = new QLabel(
        "Enter the message and a password to encrypt with GPG (symmetric, ASCII-armored).\n"
        "The encrypted text will appear below.",
        dlg
        );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    // Plaintext input
    QTextEdit *plainTextEdit = new QTextEdit(dlg);
    plainTextEdit->setPlaceholderText("Enter text to encrypt...");
    layout->addWidget(plainTextEdit);

    // Password entry row (two fields + generate button)
    QHBoxLayout *passLayout = new QHBoxLayout;

    QLabel *passLabel1 = new QLabel("Password:", dlg);
    QLineEdit *passEdit1 = new QLineEdit(dlg);
    passEdit1->setEchoMode(QLineEdit::Password);
    passEdit1->setPlaceholderText("Enter password");

    QLabel *passLabel2 = new QLabel("Confirm:", dlg);
    QLineEdit *passEdit2 = new QLineEdit(dlg);
    passEdit2->setEchoMode(QLineEdit::Password);
    passEdit2->setPlaceholderText("Re-enter password");

    QPushButton *generateBtn = new QPushButton("Generate Password", dlg);

    passLayout->addWidget(passLabel1);
    passLayout->addWidget(passEdit1);
    passLayout->addSpacing(20);
    passLayout->addWidget(passLabel2);
    passLayout->addWidget(passEdit2);
    passLayout->addSpacing(20);
    passLayout->addWidget(generateBtn);

    layout->addLayout(passLayout);

    // Encrypt button row (aligned right)
    QHBoxLayout *encryptLayout = new QHBoxLayout;
    encryptLayout->addStretch();
    QPushButton *encryptBtn = new QPushButton("&Encrypt", dlg);
    encryptLayout->addWidget(encryptBtn);
    layout->addLayout(encryptLayout);

    // Encrypted output
    QTextEdit *encryptedTextEdit = new QTextEdit(dlg);
    encryptedTextEdit->setReadOnly(true);
    encryptedTextEdit->setPlaceholderText("Encrypted text will appear here...");
    layout->addWidget(encryptedTextEdit);

    // Copy + Close buttons side by side
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *copyBtn  = new QPushButton("Copy", dlg);
    QPushButton *closeBtn = new QPushButton("&Close", dlg);
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Generate password action
    connect(generateBtn, &QPushButton::clicked, this, [this, passEdit1, passEdit2]() {
        // Launch your existing generator dialog
        this->on_actionGenerate_Password_triggered();

        // Optional: if you want to auto-fill the generated password into both fields,
        // you can modify on_actionGenerate_Password_triggered() to return the string
        // and then set passEdit1->setText(pwd); passEdit2->setText(pwd);
    });

    // Encrypt action
    connect(encryptBtn, &QPushButton::clicked, this,
            [plainTextEdit, passEdit1, passEdit2, encryptedTextEdit]() {
                const QString inputText = plainTextEdit->toPlainText();
                const QString pass1 = passEdit1->text();
                const QString pass2 = passEdit2->text();

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
                    QMessageBox msgBox;
                    msgBox.setIcon(QMessageBox::Warning);
                    msgBox.setWindowTitle("Weak Password");
                    msgBox.setText(
                        "Your password can be easily cracked in a short time.\n\n"
                        "A safe password should be at least 10 characters long and contain:\n"
                        "• Lowercase letters\n"
                        "• Uppercase letters\n"
                        "• Digits\n"
                        "• Special characters\n\n"
                        "Click OK to ignore this advice and continue."
                        );
                    msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
                    msgBox.setDefaultButton(QMessageBox::Cancel);

                    if (msgBox.exec() == QMessageBox::Cancel) {
                        return;    // user backed out
                    }
                }

                // Prepare gpg process
                QApplication::setOverrideCursor(Qt::WaitCursor);
                QApplication::processEvents();
                QProcess process;
                QStringList args;
                args << "--batch"
                     << "--yes"
                     << "--pinentry-mode" << "loopback"
                     << "--symmetric"
                     << "-a"
                     << "--passphrase" << pass1;

                process.start("gpg", args);
                if (!process.waitForStarted()) {
                    QApplication::restoreOverrideCursor();
                    QApplication::processEvents();
                    QMessageBox::critical(nullptr, "Error", "Failed to start gpg process.");
                    return;
                }

                process.write(inputText.toUtf8());
                process.closeWriteChannel();

                if (!process.waitForFinished()) {
                    QApplication::restoreOverrideCursor();
                    QApplication::processEvents();
                    QMessageBox::critical(nullptr, "Error", "gpg process did not finish.");
                    return;
                }

                const QString encryptedOutput = process.readAllStandardOutput();
                const QString errorOutput = process.readAllStandardError();

                if (!encryptedOutput.isEmpty()) {
                    encryptedTextEdit->setPlainText(encryptedOutput);
                } else {
                    encryptedTextEdit->setPlainText("Encryption failed:\n" + errorOutput);
                }
                QApplication::restoreOverrideCursor();
                QApplication::processEvents();
            });

    // Copy action
    connect(copyBtn, &QPushButton::clicked, this,
            [encryptedTextEdit]() {
                const QString text = encryptedTextEdit->toPlainText();
                if (!text.isEmpty()) {
                    QClipboard *clipboard = QGuiApplication::clipboard();
                    clipboard->setText(text);
                    QMessageBox::information(nullptr, "Copied", "Encrypted text copied to clipboard.");
                } else {
                    QMessageBox::warning(nullptr, "No Text", "There is no encrypted text to copy.");
                }
            });

    dlg->setLayout(layout);
    dlg->resize(700, 550);
    dlg->exec();
}

void MainWindow::on_actionDecrypt_message_triggered()
{
    /*
     * Decrypt Message Dialog (GPG symmetric, ASCII-armored)
     * Uses --pinentry-mode loopback to pass the dialog-entered password to gpg.
     */

    // Dialog and layout
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(ui->actionDecrypt_message->text());
    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Instruction
    QLabel *instruction = new QLabel(
        "Paste the encrypted message below and enter the password used to encrypt it.\n"
        "The decrypted plaintext will appear underneath.",
        dlg
        );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    // Encrypted input
    QTextEdit *encryptedInputEdit = new QTextEdit(dlg);
    encryptedInputEdit->setPlaceholderText("Paste encrypted text here...");
    layout->addWidget(encryptedInputEdit);

    // Password entry row (single field + decrypt button aligned right)
    QHBoxLayout *passLayout = new QHBoxLayout;
    QLabel *passLabel = new QLabel("Password:", dlg);
    QLineEdit *passEdit = new QLineEdit(dlg);
    passEdit->setEchoMode(QLineEdit::Password);
    passEdit->setPlaceholderText("Enter password");

    QPushButton *decryptBtn = new QPushButton("&Decrypt", dlg);

    passLayout->addWidget(passLabel);
    passLayout->addWidget(passEdit);

    // Add stretch here to push the button to the far right
    passLayout->addStretch();

    passLayout->addWidget(decryptBtn);

    layout->addLayout(passLayout);


    // Decrypted output
    QTextEdit *decryptedTextEdit = new QTextEdit(dlg);
    decryptedTextEdit->setReadOnly(true);
    decryptedTextEdit->setPlaceholderText("Decrypted text will appear here...");
    layout->addWidget(decryptedTextEdit);

    // Copy + Close buttons side by side (aligned right)
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton *copyBtn  = new QPushButton("Copy", dlg);
    QPushButton *closeBtn = new QPushButton("&Close", dlg);
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Decrypt action
    connect(decryptBtn, &QPushButton::clicked, this,
            [encryptedInputEdit, passEdit, decryptedTextEdit]() {
                const QString encryptedText = encryptedInputEdit->toPlainText();
                const QString pass = passEdit->text();

                if (encryptedText.isEmpty()) {
                    QMessageBox::warning(nullptr, "No Input", "Please paste some encrypted text.");
                    return;
                }
                if (pass.isEmpty()) {
                    QMessageBox::warning(nullptr, "No Password", "Please enter a password.");
                    return;
                }

                // Prepare gpg process for decryption
                QProcess process;
                QStringList args;
                args << "--batch"
                     << "--yes"
                     << "--pinentry-mode" << "loopback"
                     << "--decrypt"
                     << "--passphrase" << pass;

                process.start("gpg", args);
                if (!process.waitForStarted()) {
                    QMessageBox::critical(nullptr, "Error", "Failed to start gpg process.");
                    return;
                }

                process.write(encryptedText.toUtf8());
                process.closeWriteChannel();

                if (!process.waitForFinished()) {
                    QMessageBox::critical(nullptr, "Error", "gpg process did not finish.");
                    return;
                }

                const QString decryptedOutput = process.readAllStandardOutput();
                const QString errorOutput = process.readAllStandardError();

                if (!decryptedOutput.isEmpty()) {
                    decryptedTextEdit->setPlainText(decryptedOutput);
                } else {
                    decryptedTextEdit->setPlainText("Decryption failed:\n" + errorOutput);
                }
            });

    // Copy action
    connect(copyBtn, &QPushButton::clicked, this,
            [decryptedTextEdit]() {
                const QString text = decryptedTextEdit->toPlainText();
                if (!text.isEmpty()) {
                    QClipboard *clipboard = QGuiApplication::clipboard();
                    clipboard->setText(text);
                    QMessageBox::information(nullptr, "Copied", "Decrypted text copied to clipboard.");
                } else {
                    QMessageBox::warning(nullptr, "No Text", "There is no decrypted text to copy.");
                }
            });

    dlg->setLayout(layout);
    dlg->resize(700, 500);
    dlg->exec();
}


void MainWindow::on_actionEncrypt_File_triggered()
{
    /*
     * Encrypt File Dialog (GPG symmetric, binary output)
     * Uses --pinentry-mode loopback to pass the dialog-entered password to gpg.
     */

    // Prompt for file
    QString inputFile = QFileDialog::getOpenFileName(
        this,
        tr("Select File to Encrypt"),
        QString(),
        tr("All Files (*)")
        );
    if (inputFile.isEmpty())
        return; // user cancelled

    // Prompt for password
    QDialog passDlg(this);
    passDlg.setWindowTitle("Enter Password");
    QVBoxLayout layout(&passDlg);

    QLabel label("Enter password to encrypt the file:", &passDlg);
    layout.addWidget(&label);

    QLineEdit passEdit(&passDlg);
    passEdit.setEchoMode(QLineEdit::Password);
    layout.addWidget(&passEdit);

    QHBoxLayout btnLayout;
    QPushButton okBtn("&OK", &passDlg);
    QPushButton cancelBtn("&Cancel", &passDlg);
    btnLayout.addStretch();
    btnLayout.addWidget(&okBtn);
    btnLayout.addWidget(&cancelBtn);
    layout.addLayout(&btnLayout);

    QObject::connect(&okBtn, &QPushButton::clicked, &passDlg, &QDialog::accept);
    QObject::connect(&cancelBtn, &QPushButton::clicked, &passDlg, &QDialog::reject);

    if (passDlg.exec() != QDialog::Accepted)
        return; // user cancelled

    QString password = passEdit.text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "No Password", "You must enter a password.");
        return;
    }

    // Prepare output file name
    QString outputFile = inputFile + ".gpg";
    ui->statusbar->showMessage(QString("Encrypting %1, in the background..").arg(inputFile,10));

    // Create QProcess on heap so it lives until finished
    QProcess *process = new QProcess(this);

    QStringList args;
    args << "--batch"
         << "--yes"
         << "--pinentry-mode" << "loopback"
         << "--symmetric"
         << "--passphrase" << password
         << "-o" << outputFile
         << inputFile;

    // Connect finished signal
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, outputFile](int exitCode, QProcess::ExitStatus status) {
                QString errorOutput = process->readAllStandardError();
                if (status == QProcess::NormalExit && exitCode == 0) {
                    QMessageBox::information(this, "Success",
                                             "File encrypted successfully:\n" + outputFile);
                } else {
                    QMessageBox::critical(this, "Error",
                                          "Encryption failed:\n" + errorOutput);
                }
                process->deleteLater(); // clean up
            });

    // Handle start errors
    connect(process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
                QMessageBox::critical(this, "Error", "Failed to start gpg process.");
            });

    process->start("gpg", args);
}


void MainWindow::on_actionDecrypt_File_triggered()
{
    QString inputFile = QFileDialog::getOpenFileName(
        this,
        tr("Select File to Decrypt"),
        QString(),
        tr("Encrypted Files (*.gpg);;All Files (*)")
        );
    if (inputFile.isEmpty())
        return;

    // Prompt for password
    QDialog passDlg(this);
    passDlg.setWindowTitle("Enter Password");
    QVBoxLayout layout(&passDlg);
    QLabel label("Enter password to decrypt the file:", &passDlg);
    QLineEdit passEdit(&passDlg);
    passEdit.setEchoMode(QLineEdit::Password);
    QPushButton okBtn("&OK", &passDlg);
    QPushButton cancelBtn("&Cancel", &passDlg);

    QHBoxLayout btnLayout;
    btnLayout.addStretch();
    btnLayout.addWidget(&okBtn);
    btnLayout.addWidget(&cancelBtn);

    layout.addWidget(&label);
    layout.addWidget(&passEdit);
    layout.addLayout(&btnLayout);

    QObject::connect(&okBtn, &QPushButton::clicked, &passDlg, &QDialog::accept);
    QObject::connect(&cancelBtn, &QPushButton::clicked, &passDlg, &QDialog::reject);

    if (passDlg.exec() != QDialog::Accepted)
        return;

    QString password = passEdit.text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "No Password", "You must enter a password.");
        return;
    }

    ui->statusbar->showMessage(QString("Decrypting %1 in the background...").arg(inputFile), 0);

    // Create QProcess on heap so it lives until finished
    QProcess *process = new QProcess(this);

    QStringList args;
    args << "--batch"
         << "--yes"
         << "--pinentry-mode" << "loopback"
         << "--decrypt"
         << "--passphrase" << password
         << "--use-embedded-filename"
         << inputFile;

    // Connect finished signal
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus status) {
                QString errorOutput = process->readAllStandardError();
                if (status == QProcess::NormalExit && exitCode == 0) {
                    ui->statusbar->showMessage("Decryption complete", 5000);
                    QMessageBox::information(this, "Success",
                                             "File decrypted successfully using embedded filename.");
                } else {
                    ui->statusbar->showMessage("Decryption failed", 5000);
                    QMessageBox::critical(this, "Error",
                                          "Decryption failed:\n" + errorOutput);
                }
                process->deleteLater(); // clean up
            });

    // Handle start errors
    connect(process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
                ui->statusbar->showMessage("Failed to start gpg process", 5000);
                QMessageBox::critical(this, "Error", "Failed to start gpg process.");
            });

    process->start("gpg", args);
}

void MainWindow::on_actionOpen_Database_triggered()
{
    // Prompt user
    QString fileName = QFileDialog::getOpenFileName(
        this,
        ui->actionOpen_Database->text(),
        "",
        "Password Database (*.pwd)"
        );
    if (!fileName.isEmpty())
        openDatabase(fileName);
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
            QMessageBox::critical(this, tr("Error"),
                                  tr("This is not a valid passwords database (missing app_info)."));
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
    if (Settings::getBackupDatabase()) {
        QFileInfo fi(fileName);
        QString backupDirPath = fi.absolutePath() + "/backups";
        QDir backupDir(backupDirPath);
        if (!backupDir.exists()) {
            backupDir.mkpath(".");
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString backupPath = backupDirPath + "/" + fi.completeBaseName() + "_" + timestamp;

        if (!QFile::copy(fileName, backupPath)) {
            qWarning().noquote()
            << tr("Failed to create backup at %1:").arg(backupDirPath)
            << QFile(fileName).errorString();
        } else {
            qDebug() << "Backup created as:" << backupPath;
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
    QString connName = QUuid::createUuid().toString();
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
            qDebug() << "Search failed:" << query.lastError().text();
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
            }

            results.append(r);
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (results.isEmpty()) {
        QMessageBox::information(this, "Search", "No matches found.");
        ui->lineEdit->setFocus();
        ui->lineEdit->selectAll();
        return;
    }

    if (results.size() == 1) {
        selectInTreeWidgets(results.first().categoryId, results.first().id);
    } else {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Search Results"));

        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        QTableWidget *table = new QTableWidget(results.size(), 3, &dlg);
        table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Last Viewed") });
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->verticalHeader()->setDefaultSectionSize(20);

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
            }
        }
    }

    ui->lineEdit->clear();
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


void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}


QString MainWindow::buildItemPath(QTreeWidgetItem *item) const
{
    QStringList parts;
    QTreeWidgetItem *current = item;
    while (current) {
        parts.prepend(current->text(0)); // column 0 holds the name
        current = current->parent();
    }
    return parts.join(Settings::getPathSeparator()); // or " > " if you prefer
}


void MainWindow::populateBookmarksMenu()
{
    QString connName = QUuid::createUuid().toString();

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
        if (Settings::getMaxRecentResults()>0)
            ui->menuBookmarks->addAction(ui->actionRecent);
        if (Settings::getMaxPopularResults()>0)
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
            qWarning() << "Failed to execute query:" << query.lastError().text();
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
                pathTip = buildItemPath(categoryItem) + Settings::getPathSeparator() + appName;
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
    qDebug() << "Checking item text(0):" << item->text(0)
             << "mapped id:" << idFromName;

    if (idFromName == categoryId) {
        qDebug() << "Match found!";
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
    QString connName = QUuid::createUuid().toString();
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
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);

    if (categoryId == -1) {
        QMessageBox::information(this, "Search", "Application not found in database.");
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
        QMessageBox::information(this, "Search", "Category not found in tree.");
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

    QMessageBox::information(this, "Search", "Application not found in category.");
}






void MainWindow::initDb()
{
    /*
     * One time DB tasks and their initilizations goes here.
     */
    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
            return;
        }

        /*
         * Create a nice looking app key
         */
        QString appKeyStr;

        for (int i = 0; i < 5; ++i) {
            QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
            uuidStr.remove('-');
            appKeyStr += uuidStr;
        }

        QSqlQuery query(db);
        query.prepare(R"(
    INSERT OR IGNORE INTO app_info (key, value)
    VALUES (:app_key, :guid)
)");
        query.bindValue(":app_key", "app_key");
        query.bindValue(":guid", appKeyStr.toUtf8().toBase64());
        if (!query.exec())
        {
            qDebug() << "initDb() has failed.";
        }

        query.prepare(R"(
    INSERT OR IGNORE INTO app_info (key, value)
    VALUES (:db_create, :dt)
)");
        query.bindValue(":db_create", "db_create");
        query.bindValue(":dt", QDateTime::currentDateTime());
        if (!query.exec())
        {
            qDebug() << "initDb() has failed.";
        }

        query.prepare(R"(
            SELECT value
            FROM app_info
            WHERE key = :app_key
        )");
        query.bindValue(":app_key", "app_key");

        if (!query.exec()) {
            qDebug() << tr("Failed to read app_key") << query.lastError().text();
        } else if (query.next()) {
            this->appKey = QByteArray::fromBase64(query.value(0).toString().toUtf8());
        } else {
            qDebug() << tr("No app_key found in app_info table.");
        }
    }
    QSqlDatabase::removeDatabase(connName);

    /*
 * Check if keys exist
 * Having a key linked in the keys table is a essential
 * requirement for operation of this program.
 */

    QList<KeyEntry> keys = fetchKeys();
    if (keys.isEmpty()) {
        QApplication::restoreOverrideCursor();
        QMessageBox::StandardButton reply =
            QMessageBox::question(this,
                                  tr("No Keys Configured"),
                                  tr("You must configure at least one GPG private key "
                                     "fingerprint before saving passwords.\n\n"
                                     "Would you like to add one now?"),
                                  QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // Open the key setup dialog/action immediately
            on_actionKey_List_triggered();
        } else {
            QMessageBox::information(this,
                                     tr("Keys Required"),
                                     tr("Without a configured key, you will not be able "
                                        "to create or edit encrypted passwords."));
        }
    }

}

void MainWindow::setBookmark(bool checked)
{
    auto selected = ui->treeWidget_2->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Bookmark", "No item selected.");
        return;
    }

    int id = selected.first()->data(0,Qt::UserRole).toInt();
    QString connName = QUuid::createUuid().toString();

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
                          "VALUES (:application_id, :username)");
            query.bindValue(":application_id", id);
            query.bindValue(":username", userName);
        } else {
            query.prepare("DELETE FROM favourite "
                          "WHERE application_id = :application_id AND username = :username");
            query.bindValue(":application_id", id);
            query.bindValue(":username", userName);
        }

        if (!query.exec()) {
            QMessageBox::critical(this, "Bookmark", query.lastError().databaseText());
        }

        //this->populateBookmarksMenu();
    }

    QSqlDatabase::removeDatabase(connName);
}



void MainWindow::on_actionDelete_Password_triggered()
{
    // The keyword the user must type to confirm
    const QString confirmationKeyword = "DELETE";

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
                                                  "Incorrect Confirmation",
                                                  "You must type \"" + confirmationKeyword + "\" exactly to proceed.");
                         }
                     });

    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Show dialog and check result
    if (dialog.exec() == QDialog::Accepted) {
        // Perform the deletion here
        QString connName = QUuid::createUuid().toString();
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (!db.open()) {
                QMessageBox::critical(this, "", db.lastError().text());
                return;
            }

            if (!Settings::verifyDeleteAllowed(db, this)) {
                QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
                return; // bail out early
            }

            // proceed with delete
            QSqlQuery pragma(db);
            pragma.exec("PRAGMA foreign_keys = ON;");

            QSqlQuery query(db);
            query.prepare("DELETE FROM application WHERE id = :id");
            query.bindValue(":id",ui->treeWidget_2->selectedItems().first()->data(0,Qt::UserRole).toInt());
            if (!query.exec())
            {
                QMessageBox::critical(this,"",query.lastError().text());
            } else
            {
                delete ui->treeWidget_2->selectedItems().first();
            }
        }
        QSqlDatabase::removeDatabase(connName);
    }
}



void MainWindow::on_actionDelete_Category_triggered()
{
    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "Database Error", db.lastError().text());
            return;
        }

        if (!Settings::verifyDeleteAllowed(db, this)) {
            QMessageBox::warning(this, tr("Error"), tr("Delete not permitted."));
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
                                     "",
                                     "This category cannot be deleted because items are still assigned to it.");
            } else {
                QMessageBox::critical(this,
                                      "",
                                      err.text());
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
    QString connName = QUuid::createUuid().toString();
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
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
                          ).arg(Settings::getMaxPopularResults());

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            qDebug() << "Popular query failed:" << query.lastError().text();
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id         = query.value("application_id").toInt();
            r.categoryId = query.value("category_id").toInt();
            r.appName    = DataObfuscator::deobfuscate(query.value("application_name").toString(), this->appKey);

            // Build full category path
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);

            // Just store the numeric view count
            r.description = query.value("view_count").toString();

            results.append(r);
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (Q_UNLIKELY(results.isEmpty())) {
        QMessageBox::information(this, ui->actionPopular->text(), "No popular applications found.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(ui->actionPopular->text());

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QTableWidget *table = new QTableWidget(results.size(), 3, &dlg);
    table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Views") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(20);

    for (int row = 0; row < results.size(); ++row) {
        QTableWidgetItem *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole, results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        // Views column as integer
        QTableWidgetItem *viewsItem = new QTableWidgetItem(results[row].description);
        viewsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, 2, viewsItem);
    }

    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSortingEnabled(false);
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
        }
    }
}

void MainWindow::searchRecent()
{
    QString connName = QUuid::createUuid().toString();
    QList<SearchResult> results;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (!db.open()) {
            QMessageBox::critical(this, "", db.lastError().text());
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
            "LIMIT %1;").arg(Settings::getMaxRecentResults());

        QSqlQuery query(db);
        if (!query.exec(sql)) {
            qDebug() << "Recent query failed:" << query.lastError().text();
            return;
        }

        while (query.next()) {
            SearchResult r;
            r.id           = query.value("application_id").toInt();
            r.categoryId   = query.value("category_id").toInt();
            r.appName      = DataObfuscator::deobfuscate(query.value("application_name").toString(), this->appKey);

            // Build and deobfuscate full category path segment-by-segment
            r.categoryName = buildCategoryPath(r.categoryId, this->appKey, db);

            // Only show the unix datetime stamp in "Last Viewed"
            QString dt = query.value("dt").toString();
            r.description = dt;

            results.append(r);
        }


    }
    QSqlDatabase::removeDatabase(connName);

    if (Q_UNLIKELY(results.isEmpty())) {
        QMessageBox::information(this, ui->actionRecent->text(), "No recent applications found.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(ui->actionRecent->text());

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QTableWidget *table = new QTableWidget(results.size(), 3, &dlg);
    table->setHorizontalHeaderLabels({ tr("Application"), tr("Category"), tr("Last Viewed") });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setDefaultSectionSize(20);

    for (int row = 0; row < results.size(); ++row) {
        QTableWidgetItem *appItem = new QTableWidgetItem(results[row].appName);
        appItem->setData(Qt::UserRole, results[row].id);
        appItem->setData(Qt::UserRole + 1, results[row].categoryId);
        table->setItem(row, 0, appItem);

        // Category column (index 1)
        table->setItem(row, 1, new QTableWidgetItem(results[row].categoryName));

        // Last Viewed column (index 2) – format Unix timestamp
        qint64 unixTime = results[row].description.toLongLong();
        QDateTime ts = QDateTime::fromSecsSinceEpoch(unixTime);
        QString formatted = ts.toString(Qt::TextDate);
        table->setItem(row, 2, new QTableWidgetItem(formatted));
    }

    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSortingEnabled(false);
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
        }
    }
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

    return parts.join(Settings::getPathSeparator());
}

void MainWindow::on_actionEdit_Password_triggered()
{
    QTreeWidgetItem *item = ui->treeWidget_2->currentItem();
    if (!item) return;

    if (Settings::getKillGpgAgent()) {
        killGpgAgent();
    }

    QString connName = QUuid::createUuid().toString();
    QByteArray data;

    // --- Step 1: Retrieve encrypted data from DB ---
    ui->statusbar->showMessage("Reading database..");
    QApplication::processEvents();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT data FROM application WHERE id = :id");
            query.bindValue(":id", item->data(0,Qt::UserRole).toInt());
            if (query.exec() && query.first()) {
                data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
                if (showDebugMessages) {
                    QMessageBox::information(this, "Encrypted data", data);
                }
            } else {
                qDebug() << "Query failed:" << query.lastError().text();
            }
        } else {
            qDebug() << db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt asynchronously ---
    ui->statusbar->showMessage("Decrypting data..");
    QApplication::processEvents();

    QProcess *gpg = new QProcess(this);
    gpg->setProcessChannelMode(QProcess::SeparateChannels);

    connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
        gpg->write(data);
        data.fill(0);
        gpg->closeWriteChannel();
    });

    // Handle stdout (decrypted JSON)
    connect(gpg, &QProcess::readyReadStandardOutput, this, [this, gpg, item]() {
        QByteArray decrypted_data = gpg->readAllStandardOutput();
        if (!decrypted_data.isEmpty()) {
            ui->statusbar->clearMessage();

            if (showDebugMessages) {
                QMessageBox::information(this, "Decrypted JSON", decrypted_data);
            }

            // --- Step 3: Populate dialog with decrypted JSON ---
            QJsonDocument doc = QJsonDocument::fromJson(decrypted_data);
            QJsonObject obj = doc.object();

            NewPasswordDialog dlg(this);
            dlg.setWindowTitle("Edit Password");

            QList<KeyEntry> keys = fetchKeys();
            dlg.setKeys(keys);

            // Fill dialog fields from JSON
            dlg.AppName       = obj.value("private_name").toString();
            dlg.PublicAppName = ui->treeWidget_2->selectedItems().first()->text(0);
            dlg.Description   = obj.value("description").toString();
            dlg.URL   = obj.value("url").toString();
            dlg.openPassword();

            // --- Traverse credentials ---
            QJsonArray creds = obj.value("credentials").toArray();
            for (int i = 0; i < creds.size(); ++i) {
                QJsonObject credObj = creds.at(i).toObject();
                QString username  = credObj.value("username").toString();
                QString password  = credObj.value("password").toString();
                QString secretOpt = credObj.value("secretOptCode").toString();
                int length        = credObj.value("length").toInt();

                qDebug() << "Credential:"
                         << "username=" << username
                         << "password=" << password
                         << "secretOptCode=" << secretOpt
                         << "length=" << length;

                dlg.openCredentials(username, password, secretOpt, length);
            }


            // --- Traverse notes ---
            QJsonArray notes = obj.value("notes").toArray();
            for (const QJsonValue &val : std::as_const(notes)) {
                QJsonObject noteObj = val.toObject();
                QString body  = noteObj.value("content").toString();
                dlg.openNote(body);
            }



            if (dlg.exec() == QDialog::Accepted) {
                QByteArray newJson = dlg.toJson();

                QString tempFile = QDir::tempPath() + "/edit.asc";
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
                    QFile::remove(tempFile);
                    {
                    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","sqlEditPassword");
                    db.setDatabaseName(qApp->property("dbFile").toString());
                    if (db.open()) {
                        // --- Begin transaction ---
                        if (!db.transaction()) {
                            qDebug() << "Failed to start transaction:" << db.lastError().text();
                        }

                        bool ok = true;

                        // Update application
                        QSqlQuery update(db);
                        update.prepare(R"(
                UPDATE application
                SET application_name = :name,
                    data = :data
                WHERE id = :id)");
                        update.bindValue(":name", DataObfuscator::obfuscate(dlg.PublicAppName, appKey));
                        update.bindValue(":data", DataObfuscator::obfuscate(QString::fromUtf8(newEncrypted), appKey));
                        update.bindValue(":id", item->data(0, Qt::UserRole).toInt());

                        if (!update.exec()) {
                            qDebug() << "Update failed:" << update.lastError().text();
                            ok = false;
                        }

                        // Insert audit record
                        if (ok) {
                            QSqlQuery audit(db);
                            audit.prepare(R"(
                    INSERT INTO application_views_audit(application_id, dt, "user", "host", action, audit_time)
                    VALUES (:appId, strftime('%s','now'), :user, :host, 'EDITED', strftime('%s','now'))
                )");
                            audit.bindValue(":appId", item->data(0, Qt::UserRole).toInt());
                            audit.bindValue(":user", this->userName);
                            audit.bindValue(":host", QSysInfo::machineHostName());

                            if (!audit.exec()) {
                                qDebug() << "Audit insert failed:" << audit.lastError().text();
                                ok = false;
                            }
                        }

                        // 🔹 Refresh tokens
                        if (ok) {
                            int appId = item->data(0, Qt::UserRole).toInt();

                            // Delete old tokens
                            QSqlQuery deleteTokens(db);
                            deleteTokens.prepare("DELETE FROM application_tokens WHERE application_id = ?");
                            deleteTokens.addBindValue(appId);
                            if (!deleteTokens.exec()) {
                                qDebug() << "Token delete failed:" << deleteTokens.lastError().text();
                                ok = false;
                            }

                            // Insert new tokens
                            if (ok) {
                                int appId = item->data(0, Qt::UserRole).toInt();

                                // Normalize: lowercase, replace any non-letter/digit with a space, then collapse spaces
                                static const QRegularExpression nonAlphaNum("[^\\p{L}\\p{N}]+");

                                QString normalized = dlg.PublicAppName.toLower();
                                normalized.replace(nonAlphaNum, " ");
                                normalized = normalized.trimmed();

                                // Split on whitespace
                                static const QRegularExpression whitespaceRe("\\s+");
                                QStringList tokens = normalized.split(whitespaceRe, Qt::SkipEmptyParts);

                                for (const QString &token : std::as_const(tokens)) {
                                    QByteArray hash = QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256).toHex();
                                    QSqlQuery insertToken(db);
                                    insertToken.prepare("INSERT INTO application_tokens (application_id, token_hash) VALUES (?, ?)");
                                    insertToken.addBindValue(appId);
                                    insertToken.addBindValue(QString(hash));
                                    if (!insertToken.exec()) {
                                        qDebug() << "Token insert failed:" << insertToken.lastError().text();
                                        ok = false;
                                        break;
                                    }
                                }
                            }

                        }

                        // Commit or rollback
                        if (ok) {
                            if (!db.commit())
                                qDebug() << "Commit failed:" << db.lastError().text();
                        } else {
                            if (!db.rollback())
                                qDebug() << "Rollback failed:" << db.lastError().text();
                        }
                    }
                    db.close();
                    }
                    QSqlDatabase::removeDatabase("sqlEditPassword");
                }
                //refresh the view
                //any rename shows immediately
                QTreeWidgetItem *current = ui->treeWidget->currentItem();
                if (current) {
                    this->openCategory(current, 0);
                }
            }
            decrypted_data.fill(0);
        }
    });

    // Handle stderr
    connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg]() {
        QByteArray errors = gpg->readAllStandardError();
        if (!errors.isEmpty()) {
            ui->statusbar->showMessage(QString::fromUtf8(errors));
            qDebug().noquote() << "GPG stderr:" << errors;
        }
    });

    // Handle completion
    connect(gpg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, gpg, item](int exitCode, QProcess::ExitStatus status) {
                if (status != QProcess::NormalExit || exitCode != 0) {
                    openedCredentialID = -1;
                } else {
                    openedCredentialID = item->text(1).toInt();
                }
                gpg->deleteLater();

                QApplication::restoreOverrideCursor();
                QApplication::processEvents();
            });

    gpg->start("gpg", QStringList() << "--decrypt");
}

void MainWindow::on_actionExport_Password_triggered()
{
    QTreeWidgetItem *item = ui->treeWidget_2->currentItem();
    if (!item) return;

    // --- WARNING prompt ---
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        tr("WARNING, WARNING, WARNING"),
        tr("<b><span style='color:red;'>If you continue, you will create an UNENCRYPTED copy of your password data on disk.</span></b><br><br>"
           "This is how password leaks occur.<br><br>"
           "This file will <b>NOT</b> be protected by GPG.<br><br>"
           "Do you really want to continue?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );


    if (reply != QMessageBox::Yes) {
        return; // user chose No
    }

    if (Settings::getKillGpgAgent()) {
        killGpgAgent();
    }

    QString connName = QUuid::createUuid().toString();
    QByteArray data;

    // --- Step 1: Retrieve encrypted data from DB ---
    ui->statusbar->showMessage("Reading database..");
    QApplication::processEvents();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT data FROM application WHERE id = :id");
            query.bindValue(":id", item->data(0,Qt::UserRole).toInt());
            if (query.exec() && query.first()) {
                data = DataObfuscator::deobfuscate(query.value(0).toString(), appKey).toUtf8();
                if (showDebugMessages) {
                    QMessageBox::information(this, "Encrypted data", data);
                }
            } else {
                qDebug() << "Query failed:" << query.lastError().text();
            }
        } else {
            qDebug() << db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    ui->statusbar->clearMessage();

    // --- Step 2: Decrypt asynchronously ---
    ui->statusbar->showMessage("Decrypting data..");
    QApplication::processEvents();

    QProcess *gpg = new QProcess(this);
    gpg->setProcessChannelMode(QProcess::SeparateChannels);

    connect(gpg, &QProcess::started, this, [gpg, data]() mutable {
        gpg->write(data);
        data.fill(0);
        gpg->closeWriteChannel();
    });

    // Handle stdout (decrypted JSON)
    connect(gpg, &QProcess::readyReadStandardOutput, this, [this, gpg, item]() {
        QByteArray decrypted_data = gpg->readAllStandardOutput();
        if (!decrypted_data.isEmpty()) {
            ui->statusbar->clearMessage();

            // Build suggested filename: <item text>_<date>.json
            QString baseName = item->text(0); // column 0 usually holds the display name
            QString dateStr  = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            QString suggestedName = QDir::homePath() + "/" + baseName + "_" + dateStr + ".json";

            // Prompt user with Save As dialog, using suggestedName as default
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
                    QMessageBox::information(this, "Export Complete",
                                             "Decrypted JSON saved to:\n" + fileName);
                } else {
                    QMessageBox::critical(this, "Error",
                                          "Could not open file for writing:\n" + fileName);
                }
            }

            decrypted_data.fill(0);
        }
    });



    // Handle stderr
    connect(gpg, &QProcess::readyReadStandardError, this, [this, gpg]() {
        QByteArray errors = gpg->readAllStandardError();
        if (!errors.isEmpty()) {
            ui->statusbar->showMessage(QString::fromUtf8(errors));
            qDebug().noquote() << "GPG stderr:" << errors;
        }
    });

    // Handle completion
    connect(gpg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, gpg, item](int exitCode, QProcess::ExitStatus status) {
                if (status != QProcess::NormalExit || exitCode != 0) {
                    openedCredentialID = -1;
                } else {
                    openedCredentialID = item->text(1).toInt();
                }
                gpg->deleteLater();

                QApplication::restoreOverrideCursor();
                QApplication::processEvents();
            });

    gpg->start("gpg", QStringList() << "--decrypt");
}


QString getItemPath(QTreeWidgetItem *item, int column = 0)
{
    QStringList parts;
    QTreeWidgetItem *current = item;
    while (current) {
        parts.prepend(current->text(column));   // or current->data(column, Qt::UserRole).toString()
        current = current->parent();
    }
    return parts.join(Settings::getPathSeparator());
}

void MainWindow::moveCategory(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem) {
    if (!sourceItem || !targetItem) {
        qDebug() << "Invalid source or target item, ignoring drop";
        return;
    }

    int appId       = sourceItem->data(0, Qt::UserRole).toInt();
    int newCategory = targetItem->data(0, Qt::UserRole).toInt();

    qDebug() << "Moving application" << appId << "to category" << newCategory;

    // --- Step 0: Optional prompt ---
    if (Settings::getDragDropPrompt()) {
        QString msg = tr("Move entry \"%1\" to category \"%2\"?\n").arg(sourceItem->text(0), targetItem->text(0));
        if (QMessageBox::question(this, tr("Confirm Move"), msg,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes) {
            qDebug() << "User cancelled drag-drop move";
            return;
        }
    }

    // --- Step 1: Update DB ---
    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("UPDATE application SET category_id = :cat WHERE id = :id");
            query.bindValue(":cat", newCategory);
            query.bindValue(":id", appId);
            if (query.exec()) {
                // --- Step 2: Update UI ---
                auto selected = ui->treeWidget_2->selectedItems().first();
                if (selected) delete selected;
                delete sourceItem;
            } else {
                qDebug() << "Update failed:" << query.lastError().text();
                QMessageBox::critical(this, tr("Database Error"),
                                      tr("Could not update category:\n%1")
                                          .arg(query.lastError().text()));
            }
            db.close();
        } else {
            qDebug() << "DB open failed:" << db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(connName);
}

void MainWindow::importApplicationsFromFile(const QString &filePath)
{
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
    QString connName = QUuid::createUuid().toString();
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
            QFile::remove(tempFile);

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

            // --- Audit row ---
            QSqlQuery audit(db);
            audit.prepare(R"(
                INSERT INTO application_views_audit(application_id, dt, user, host, action, audit_time)
                VALUES (:id, :dt, :user, :host, 'CREATED', strftime('%s','now')))");

            audit.bindValue(":id", appId);
            audit.bindValue(":dt", QDateTime::currentSecsSinceEpoch());
            audit.bindValue(":user", userName);
            audit.bindValue(":host", QSysInfo::machineHostName());
            if (!audit.exec()) {
                errors << tr("Audit insert failed for '%1': %2")
                .arg(publicAppName, audit.lastError().text());
                skippedCount++;
                continue;
            }

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

    // Prompt if no name provided
    if (text.isEmpty()) {
        text = QInputDialog::getText(this, tr("Category"), tr("Create a new category:")).trimmed();
        if (text.isEmpty()) {
            return;
        }
    }

    // Validate name
    static const QRegularExpression re("^[\\w\\-\\s]{1,64}$");

    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this,
                             tr("Invalid Name"),
                             tr("Category name contains invalid characters or is too long."));
        return;
    }

    // Determine parent item and id
    QTreeWidgetItem* parentItem = ui->treeWidget->currentItem();
    QVariant parentId = QVariant(QMetaType(QMetaType::Int)); // NULL by default
    if (parentItem) {
        parentId = parentItem->data(0, Qt::UserRole); // parent’s DB id
    }

    // Duplicate check in UI
    QTreeWidgetItem* scope = parentItem ? parentItem : ui->treeWidget->invisibleRootItem();
    for (int i = 0; i < scope->childCount(); ++i) {
        if (scope->child(i)->text(0) == text) {
            QMessageBox::warning(this, tr("Duplicate"),
                                 tr("A category with this name already exists here."));
            return;
        }
    }

    // DB connection
    QString connName = QUuid::createUuid().toString();
    {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(qApp->property("dbFile").toString());

    if (!db.open()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to open database:\n%1").arg(db.lastError().text()));
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    // Insert into DB
    QSqlQuery query(db);
    query.prepare("INSERT INTO categories (parent_id, text) VALUES (:parent_id, :text)");
    query.bindValue(":parent_id", parentId); // NULL if top-level
    query.bindValue(":text", DataObfuscator::obfuscate(text, this->appKey));

    if (!query.exec()) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Insert failed:\n%1").arg(query.lastError().text()));
        db.close();
        QSqlDatabase::removeDatabase(connName);
        return;
    }

    int newId = query.lastInsertId().toInt();

    // Create tree item with its OWN id
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
    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (!db.open()) {
            QMessageBox::critical(this, tr("Database Error"), tr("Failed to open database:\n%1").arg(db.lastError().text()));
            QSqlDatabase::removeDatabase(connName);
            return;
        }

        int id = item->data(0, Qt::UserRole).toInt();
        QSqlQuery query(db);
        query.prepare("UPDATE categories SET text = :text WHERE id = :id");
        query.bindValue(":text", DataObfuscator::obfuscate(text, this->appKey));
        query.bindValue(":id", id);

        if (!query.exec()) {
            QMessageBox::critical(this, tr("Database Error"), tr("Update failed:\n%1").arg(query.lastError().text()));
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



void MainWindow::on_actionAdd_Search_triggered()
{
    // Find the currently selected application in your tree
    QTreeWidgetItem *item = ui->treeWidget_2->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("Add Search Term"), tr("No application selected."));
        return;
    }

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

    connect(addBtn, &QPushButton::clicked,
            addBtn,   // context object
            [lineEdit, listWidget]() {
                QString text = lineEdit->text().trimmed();
                if (!text.isEmpty()) {
                    listWidget->addItem(text);
                    lineEdit->clear();
                }
            });

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        QString connName = QUuid::createUuid().toString();
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
    //ESC Key
    if (event->key() == Qt::Key_Escape) {
        qDebug() << "Escape pressed!";

        if (openedCredentialID == -1) {
            qDebug() << "Already closed";
            if (Settings::getAskClose()) {
                close();
            }
            event->accept();
            return; // symmetry with other branch
        } else {
            clearScrollArea();
            event->accept();
            return;
        }
    }

    //CTRL+F
    if (event->key() == Qt::Key_F && event->modifiers() == Qt::ControlModifier)
    {
        ui->lineEdit->setFocus();
        event->accept();
        return;
    }

    // Pass unhandled keys to base class
    QMainWindow::keyPressEvent(event);
}

void MainWindow::launchHelperProcess(const QString &page)
{
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
         << "--port"   << QString::number(Settings::getHelpPort());

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
                        QMessageBox::warning(nullptr, tr("Error"),
                                             QString("Failed to start: %1").arg(exePath));
                    }
                });

        helperProcess->start();
    } else {
        // Already running: just launch another instance detached
        QProcess::startDetached(exePath, args);
    }

    QApplication::restoreOverrideCursor();
}
