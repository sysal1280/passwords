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


#include "newpassworddialog.h"
#include "ui_newpassworddialog.h"

#include "integerdelegate.h"
#include "multilinedelegate.h"
#include "passworddialog.h"
#include "passwordgenerator.h"
#include "randomnoisedialog.h"
#include "settings.h"
#include "gpgcheck.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QTimer>


class PasswordDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    bool validate(const QString &text) const {
        if (text.isEmpty()) return true;
        return isStrong(text);
    }

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        if (index.column() == 1) {
            QLineEdit *line = new QLineEdit(parent);

            line->setProperty("row", index.row());
            line->setProperty("column", index.column());
            line->setProperty("handledCommitKey", false);
            line->setProperty("contextMenuActive", false);

            // Optional: disable context menu entirely
            // line->setContextMenuPolicy(Qt::NoContextMenu);

            line->installEventFilter(const_cast<PasswordDelegate*>(this));
            return line;
        }

        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    bool eventFilter(QObject *editor, QEvent *event) override
    {
        QLineEdit *line = qobject_cast<QLineEdit*>(editor);
        if (!line)
            return QStyledItemDelegate::eventFilter(editor, event);

        int row = line->property("row").toInt();
        int column = line->property("column").toInt();

        if (column != 1)
            return QStyledItemDelegate::eventFilter(editor, event);

        bool handledCommit = line->property("handledCommitKey").toBool();
        bool contextMenuActive = line->property("contextMenuActive").toBool();

        // ----------------------------------------------------
        // 0. CONTEXT MENU HANDLING (THE REAL FIX)
        // ----------------------------------------------------
        if (event->type() == QEvent::ContextMenu) {
            line->setProperty("contextMenuActive", true);
            return false; // allow menu
        }

        // When context menu closes, Qt sends FocusOut → ignore it
        if (contextMenuActive && event->type() == QEvent::FocusOut) {
            // Reset flag AFTER ignoring this FocusOut
            line->setProperty("contextMenuActive", false);
            return false; // do NOT validate
        }

        // ----------------------------------------------------
        // 1. NORMAL VALIDATION LOGIC
        // ----------------------------------------------------

        auto reopenEditor = [&](QLineEdit *line) {
            if (auto *view = qobject_cast<QAbstractItemView*>(parent())) {
                QModelIndex idx = view->model()->index(row, column);

                QMetaObject::invokeMethod(
                    view,
                    [view, idx]() {
                        view->edit(idx);
                        if (auto *line = view->findChild<QLineEdit*>()) {
                            line->setFocus();
                            line->selectAll();
                        }
                    },
                    Qt::QueuedConnection
                    );
            }
        };

        // Commit keys
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *key = static_cast<QKeyEvent*>(event);

            bool isCommitKey =
                key->key() == Qt::Key_Return ||
                key->key() == Qt::Key_Enter ||
                key->key() == Qt::Key_Tab ||
                key->key() == Qt::Key_Backtab;

            if (isCommitKey) {
                line->setProperty("handledCommitKey", true);

                if (!validate(line->text())) {
                    bool keep = warnAndContinue();
                    if (!keep) {
                        line->clear();
                        reopenEditor(line);
                        return true;
                    }
                }
            }
        }

        // FocusOut
        if (event->type() == QEvent::FocusOut) {

            if (handledCommit)
                return false;

            if (!validate(line->text())) {
                bool keep = warnAndContinue();
                if (!keep) {
                    line->clear();
                    reopenEditor(line);
                    return true;
                }
            }
        }

        return QStyledItemDelegate::eventFilter(editor, event);
    }

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
};



NewPasswordDialog::NewPasswordDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NewPasswordDialog)
{
    ui->setupUi(this);

    ui->toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    ui->toolButton->setText(tr("Generate"));
    QMenu *menu = new QMenu(ui->toolButton);
    QAction *optA = menu->addAction(QIcon(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"), tr("Generate Password"));
    optA->setShortcut(QKeySequence(Qt::Key_F5));
    QAction *optB = menu->addAction(QIcon(":/menus/glyphs/grain_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"), tr("Random Noise"));
    optB->setShortcut(QKeySequence(Qt::Key_F6));
    ui->toolButton->setMenu(menu);

    ui->tableWidgetCredentials->setItemDelegate(new PasswordDelegate(this));

    // Connect using connect()
    connect(optA, &QAction::triggered,
            this,
            [this]() {
                QStringList wordList = passwordGenerator::loadWordList(settings.getWordListFile());
                PasswordDialog::showPasswordGenerator(this, tr("Generate Password"), wordList);
            });

    connect(optB, &QAction::triggered,
            this,
            [this]() {
                RandomNoiseDialog::showRandomNoiseGenerator(this);
            });

    connect(ui->toolButton, &QToolButton::clicked,
            this, [this]() {
                QStringList wordList = passwordGenerator::loadWordList(settings.getWordListFile());
                PasswordDialog::showPasswordGenerator(this, tr("Generate Password"), wordList);
            });

    // Existing context menu setup...
    ui->tableWidgetNotes->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidgetNotes, &QTableWidget::customContextMenuRequested,
            this, &NewPasswordDialog::onNotesContextMenu);

    // Install the multi-line delegate
    ui->tableWidgetNotes->setItemDelegate(new MultiLineDelegate(ui->tableWidgetNotes));
    ui->tableWidgetNotes->verticalHeader()->setDefaultSectionSize(100);
    ui->tableWidgetNotes->setWordWrap(true);
    ui->tableWidgetNotes->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableWidgetNotes, &QTableWidget::itemChanged,
            ui->tableWidgetNotes, &QTableWidget::resizeRowsToContents);

    connect(ui->tableWidgetNotes->model(), &QAbstractItemModel::rowsInserted,
            this, [this](const QModelIndex &, int first, int last) {
                QTimer::singleShot(0, this, [this, first, last]() {
                    for (int r = first; r <= last; ++r)
                        ui->tableWidgetNotes->setRowHeight(r, 100);
                });
            });

    ui->tableWidgetCredentials->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidgetCredentials, &QTableWidget::customContextMenuRequested,
            this, &NewPasswordDialog::onCredentialsContextMenu);

    ui->tableWidgetCredentials->setItemDelegateForColumn(3, new IntegerDelegate(ui->tableWidgetCredentials));

    ui->listWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Example: connect signals from widgets to a slot that checks conditions
    connect(ui->lineEditAppName, &QLineEdit::textChanged,
            this, &NewPasswordDialog::validateForm);
    connect(ui->tableWidgetCredentials, &QTableWidget::itemChanged,
            this, &NewPasswordDialog::validateForm);
    connect(ui->tableWidgetNotes, &QTableWidget::itemChanged,
            this, &NewPasswordDialog::validateForm);

    connect(ui->tableWidgetCredentials->model(), &QAbstractItemModel::rowsInserted,
            this, &NewPasswordDialog::validateForm);
    connect(ui->tableWidgetCredentials->model(), &QAbstractItemModel::rowsRemoved,
            this, &NewPasswordDialog::validateForm);

    connect(ui->listWidget, &QListWidget::itemChanged,
            this, &NewPasswordDialog::validateForm);

    connect(ui->lineEditPublicAppName, &QLineEdit::editingFinished,
            this, &NewPasswordDialog::suggestFields);

    // In your NewPasswordDialog constructor or setup code
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        // Check that at least one item in listWidget is checked
        bool anyChecked = false;
        for (int i = 0; i < ui->listWidget->count(); ++i) {
            QListWidgetItem *item = ui->listWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                anyChecked = true;
                break;
            }
        }

        if (!anyChecked) {
            QMessageBox::warning(this,
                                 tr("Missing selection"),
                                 tr("Please check at least one key before continuing."));
            // prevent dialog from accepting
            return; // don't call accept()
        }

        // If we get here, at least one item is checked
        this->Description    = ui->lineEditDescription->text().trimmed();
        this->URL            = ui->lineEditURL->text().trimmed();
        this->AppName        = ui->lineEditAppName->text().trimmed();
        this->PublicAppName  = ui->lineEditPublicAppName->text().trimmed();

        accept(); // explicitly accept the dialog
    });

}

void NewPasswordDialog::openPassword()
{
    ui->lineEditAppName->setText(this->AppName);
    ui->lineEditPublicAppName->setText(this->PublicAppName);
    ui->lineEditDescription->setText(this->Description);
    ui->lineEditURL->setText(this->URL);
}

void NewPasswordDialog::openCredentials(QString username,
                                        QString password,
                                        QString secretOtpCode,
                                        int length)
{
    // Ensure the table has the right number of columns
    if (ui->tableWidgetCredentials->columnCount() < 4) {
        ui->tableWidgetCredentials->setColumnCount(4);
        ui->tableWidgetCredentials->setHorizontalHeaderLabels(
            {"Username", "Password", "Secret Code", "Length"});
    }

    // Add a new row at the bottom
    int row = ui->tableWidgetCredentials->rowCount();
    ui->tableWidgetCredentials->insertRow(row);

    // Populate cells
    ui->tableWidgetCredentials->setItem(row, 0, new QTableWidgetItem(username));
    ui->tableWidgetCredentials->setItem(row, 1, new QTableWidgetItem(password));
    ui->tableWidgetCredentials->setItem(row, 2, new QTableWidgetItem(secretOtpCode));
    ui->tableWidgetCredentials->setItem(row, 3, new QTableWidgetItem(QString::number(length)));

    // Optional: adjust column widths
    ui->tableWidgetCredentials->resizeColumnsToContents();

}

void NewPasswordDialog::openNote(QString note)
{
    // Ensure the table has exactly 1 column
    if (ui->tableWidgetNotes->columnCount() != 1) {
        ui->tableWidgetNotes->setColumnCount(1);
        ui->tableWidgetNotes->setHorizontalHeaderLabels({"Note"});
    }

    // Add a new row at the bottom
    int row = ui->tableWidgetNotes->rowCount();
    ui->tableWidgetNotes->insertRow(row);

    // Populate the single cell with the note text
    ui->tableWidgetNotes->setItem(row, 0, new QTableWidgetItem(note));

    // Optional: adjust column width
    ui->tableWidgetNotes->resizeColumnsToContents();
}



void NewPasswordDialog::onCredentialsContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    QAction *addRowAction    = menu.addAction("Add New Credential");
    QAction *deleteRowAction = menu.addAction("Delete Selected Credential");

    QAction *selectedAction = menu.exec(ui->tableWidgetCredentials->viewport()->mapToGlobal(pos));

    if (selectedAction == addRowAction) {
        int row = ui->tableWidgetCredentials->rowCount();
        ui->tableWidgetCredentials->insertRow(row);

        for (int col = 0; col < ui->tableWidgetCredentials->columnCount(); ++col) {
            if (col == 3) {   // column 4 (index 3)
                QTableWidgetItem *intItem = new QTableWidgetItem();
                intItem->setFlags(intItem->flags() | Qt::ItemIsEditable);
                intItem->setData(Qt::EditRole, 6); // default value
                ui->tableWidgetCredentials->setItem(row, col, intItem);
            } else {
                QTableWidgetItem *item = new QTableWidgetItem("");
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                ui->tableWidgetCredentials->setItem(row, col, item);
            }
        }

        ui->tableWidgetCredentials->setCurrentCell(row, 0);
        ui->tableWidgetCredentials->editItem(ui->tableWidgetCredentials->item(row, 0));
    }
    else if (selectedAction == deleteRowAction) {
        // 🔑 Ensure the row under the cursor is selected
        QModelIndex index = ui->tableWidgetCredentials->indexAt(pos);
        if (index.isValid()) {
            ui->tableWidgetCredentials->selectRow(index.row());
            ui->tableWidgetCredentials->removeRow(index.row());
        }
    }
}



NewPasswordDialog::~NewPasswordDialog()
{
    delete ui;
}

void NewPasswordDialog::onNotesContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    QAction *addRowAction = menu.addAction("Add New Note");
    QAction *deleteRowAction = menu.addAction("Delete Selected Note");

    QAction *selectedAction = menu.exec(ui->tableWidgetNotes->viewport()->mapToGlobal(pos));

    if (selectedAction == addRowAction) {
        int row = ui->tableWidgetNotes->rowCount();
        ui->tableWidgetNotes->insertRow(row);

        // 🔑 Create editable items for each column
        for (int col = 0; col < ui->tableWidgetNotes->columnCount(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem("");
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            ui->tableWidgetNotes->setItem(row, col, item);
        }

        // 🔑 Immediately start editing the first cell with your QTextEdit delegate
        ui->tableWidgetNotes->setCurrentCell(row, 0);
        ui->tableWidgetNotes->editItem(ui->tableWidgetNotes->item(row, 0));
    }
    else if (selectedAction == deleteRowAction) {
        int row = ui->tableWidgetNotes->currentRow();
        if (row >= 0) {
            ui->tableWidgetNotes->removeRow(row);
        }
    }
}


void NewPasswordDialog::setKeys(const QList<KeyEntry> &keys)
{
    m_keys = keys;
    ui->listWidget->clear();

    for (const auto &entry : keys) {
        // Show both label and key in the text
        QString displayText = entry.label + " (" + entry.key + ")";
        QListWidgetItem *item = new QListWidgetItem(displayText);

        // Add a checkbox
        item->setCheckState(Qt::Unchecked);

        // Store the actual key in item data for easy retrieval later
        item->setData(Qt::UserRole, entry.key);

        ui->listWidget->addItem(item);
    }
}


QByteArray NewPasswordDialog::toJson() const
{
    QJsonObject root;
    root["url"] = ui->lineEditURL->text();
    root["description"] = ui->lineEditDescription->text();
    root["private_name"] = ui->lineEditAppName->text();

    // --- Credentials ---
    QJsonArray credsArray;
    int credsRowCount = ui->tableWidgetCredentials->rowCount();

    for (int row = 0; row < credsRowCount; ++row) {
        QJsonObject cred;

        QTableWidgetItem *usernameItem = ui->tableWidgetCredentials->item(row, 0);
        QTableWidgetItem *passwordItem = ui->tableWidgetCredentials->item(row, 1);
        QTableWidgetItem *secretItem   = ui->tableWidgetCredentials->item(row, 2);
        QTableWidgetItem *lengthItem   = ui->tableWidgetCredentials->item(row, 3);

        if (!usernameItem || !passwordItem || !secretItem || !lengthItem)
            continue; // skip incomplete rows

        cred["username"]      = usernameItem->text();
        cred["password"]      = passwordItem->text();
        cred["secretOtpCode"] = secretItem->text();
        cred["length"]        = lengthItem->text().toInt();

        credsArray.append(cred);
    }
    root["credentials"] = credsArray;

    // --- Notes ---
    QJsonArray notesArray;
    int notesRowCount = ui->tableWidgetNotes->rowCount();

    for (int row = 0; row < notesRowCount; ++row) {
        QTableWidgetItem *contentItem = ui->tableWidgetNotes->item(row, 0);
        if (!contentItem)
            continue; // skip empty rows

        QJsonObject note;
        note["content"] = contentItem->text();
        notesArray.append(note);
    }
    root["notes"] = notesArray;

    // --- Finalize ---
    QJsonDocument doc(root);
    qDebug() << doc.toJson(QJsonDocument::Indented);
    return doc.toJson(QJsonDocument::Indented);
}


QStringList NewPasswordDialog::getAllKeys() const
{
    QStringList keys;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        keys << item->data(Qt::UserRole).toString();
    }
    return keys;
}

QStringList NewPasswordDialog::getCheckedKeys() const
{
    QStringList keys;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            keys << item->data(Qt::UserRole).toString();
        }
    }
    return keys;
}

void NewPasswordDialog::validateForm()
{
    // Condition 1: App name must not be empty
    if (ui->lineEditAppName->text().trimmed().isEmpty()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    // Condition 2: At least one credential row
    if (ui->tableWidgetCredentials->rowCount() == 0) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    // Condition 3: At least one checked item in listWidget
    bool hasChecked = false;
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem *item = ui->listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            hasChecked = true;
            break;
        }
    }
    if (!hasChecked) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    // Add other conditions here, each with an early return if failed...

    // If all conditions passed, enable OK
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void NewPasswordDialog::suggestFields()
{
    const QString publicName = ui->lineEditPublicAppName->text().trimmed();
    if (publicName.isEmpty())
        return;

    if (ui->lineEditAppName->text().trimmed().isEmpty())
        ui->lineEditAppName->setText(publicName);

    if (ui->lineEditDescription->text().trimmed().isEmpty())
        ui->lineEditDescription->setText(publicName);
}

