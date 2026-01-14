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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "constants.h"
#include "mainwindow.h"
#include "preferencesdialog.h"
#include "settings.h"
#include "utils.h"
#include "dbutils.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QLineEdit>
#include <QMessageBox>
#include <QResizeEvent>
#include <QResource>
#include <QSpinBox>
#include <QSqlDatabase>
#include <quuid.h>


namespace {
bool parseBool(const QVariant &v, bool fallback = false) {
    if (!v.isValid()) return fallback;

    // Already a bool?
    if (v.typeId() == QMetaType::Bool) {
        return v.toBool();
    }

    // Numeric?
    bool ok = false;
    int i = v.toInt(&ok);
    if (ok) {
        return i != 0;
    }

    // String normalization
    const QString s = v.toString().trimmed().toLower();
    if (s.isEmpty()) return fallback;

    static const QSet<QString> truths = {"1", "true", "yes", "on"};
    static const QSet<QString> falses = {"0", "false", "no", "off"};

    if (truths.contains(s)) return true;
    if (falses.contains(s)) return false;

    return fallback;
}
}

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    QStringList labels = { tr("General"), tr("Security"), tr("Passwords"), tr("Advanced")  };
    for (int i = 0; i < labels.size(); ++i) {
        ui->tabWidget->setTabText(i, labels.at(i));
    }

    ui->tabWidget->setCurrentIndex(0);

    QString wordListPath = loadWordlistResource(this, Q_FUNC_INFO);

    QDir dir(":/wordlist");
    QStringList entries = dir.entryList(QDir::Files);
    for (const QString &name : std::as_const(entries)) {
        ui->comboBoxWordList->addItem(name);
    }

    if (!QResource::unregisterResource(wordListPath))
    {
        qCritical().noquote() << Q_FUNC_INFO << "Failed to unregister " << wordListPath;
    }


    ui->labelOrchadStreetWords->clear();
    ui->labelOrchadStreetWords->setObjectName("labelOrchadStreetWords");
    ui->labelOrchadStreetWords->setTextFormat(Qt::RichText);
    ui->labelOrchadStreetWords->setOpenExternalLinks(true);

    ui->labelOrchadStreetWords->setTextInteractionFlags(
        Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse
        );

    // Inline CSS inside the HTML — the ONLY method Qt respects
    const QString linkText = tr("Learn more");

    ui->labelOrchadStreetWords->setText(
        QString(
            "<a href=\"%1\" style=\"color:#1a7bb0; text-decoration:none;\">%2</a>"
            ).arg(Passwords::WordlistUrl, linkText)
        );

    ui->comboBoxEchoMode->addItem(tr("Normal"), QLineEdit::Normal);
    ui->comboBoxEchoMode->addItem(tr("No Echo"), QLineEdit::NoEcho);
    ui->comboBoxEchoMode->addItem(tr("Password"), QLineEdit::Password);
    ui->comboBoxEchoMode->addItem(tr("Password Echo On Edit"), QLineEdit::PasswordEchoOnEdit);

    ui->tabWidget->setTabIcon(0,QIcon(":/menus/glyphs/tune_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    ui->tabWidget->setTabIcon(1,QIcon(":/menus/glyphs/verified_user_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    ui->tabWidget->setTabIcon(2,QIcon(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));
    ui->tabWidget->setTabIcon(3,QIcon(":/menus/glyphs/settings_applications_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"));

    widgetMap.insert("Main/BackupDatabase", ui->groupBox);
    widgetMap.insert("Main/AskBeforeClosing", ui->checkBoxAskClose);
    widgetMap.insert("Main/RequireChallenge", ui->checkBoxRequireChallenge);
    widgetMap.insert("Passwords/KillGPGAgent", ui->checkBoxKillGPGAgent);
    widgetMap.insert("Passwords/GeneratedPasswordLength",ui->spinBox);
    widgetMap.insert("Passwords/AutoClose",ui->spinBoxAutoClose);
    widgetMap.insert("Passwords/PathSeparator",ui->lineEditSeparator);
    widgetMap.insert("Passwords/DragDropPrompt", ui->checkboxDragDropPrompt);
    widgetMap.insert("Passwords/WordList", ui->comboBoxWordList);
    widgetMap.insert("Search/MaxRecentResults", ui->spinBoxRecent);
    widgetMap.insert("Search/MaxPopularResults", ui->spinBoxPopular);
    widgetMap.insert("Help/Port", ui->spinBoxHelpPort);
    widgetMap.insert("Help/CloseServer", ui->checkBoxCloseHelpServer);
    widgetMap.insert("Main/EchoMode", ui->comboBoxEchoMode);
    widgetMap.insert("Passwords/KillGPGAgentOnExit", ui->checkBoxKillGPGAgentExit);
    widgetMap.insert("Main/MaxBackups", ui->spinBoxMaxBackups);
    widgetMap.insert("Categories/DoubleClickOpen", ui->checkBoxDblClickCategories);

    auto removeGroupBox = [this]() {
        // Write the flag immediately
        settings.setExportedKey();

        // Remove the UI element after the window is fully constructed
        QTimer::singleShot(0, this, [this]() {
            ui->groupBox_10->hide();
            if (auto parentLayout = ui->groupBox_10->parentWidget()->layout())
                parentLayout->removeWidget(ui->groupBox_10);
            ui->groupBox_10->setParent(nullptr);
            ui->groupBox_10->deleteLater();
        });
    };

        qInfo() << Q_FUNC_INFO << "Checking for exported app key";

        const QString connectionName =
            QUuid::createUuid().toString(QUuid::WithoutBraces);

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(qApp->property("dbFile").toString());

            if (!db.open()) {
                showDbNotOpenError(this, db, Q_FUNC_INFO);
            } else {
                if (isKeyExported(this, db))
                    removeGroupBox();
            }
        }

        QSqlDatabase::removeDatabase(connectionName);

    ui->spinBox->setRange(2,10);
    ui->spinBoxAutoClose->setRange(0,600);

    loadSettings();
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &PreferencesDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &PreferencesDialog::restoreButtonClicked);
    connect(ui->groupBox, &QGroupBox::toggled,
            ui->pushButton, &QPushButton::setEnabled);

    connect(ui->groupBox, &QGroupBox::toggled,
            this, &PreferencesDialog::onBackupCheckStateChanged);

    connect(ui->pushButton, &QPushButton::clicked,
            this, &PreferencesDialog::openBackupDir);
    connect(ui->buttonBox->button(QDialogButtonBox::Help),
            &QPushButton::clicked,
            this,
            [this]() {

                checkHelpReachable([this](bool reachable) {
                    if (reachable) {
                        // Open the online help page for preferences
                        const QUrl url(getHelpBaseUrl("preferences"));
                        QDesktopServices::openUrl(url);
                    } else {
                        // Fallback to helper process
                        MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
                        if (!mw) {
                            QMessageBox::warning(
                                this,
                                tr("Help Error"),
                                tr("Help system unavailable: parent window is not MainWindow.")
                                );
                            return;
                        }

                        mw->launchHelperProcess(QStringLiteral("preferences"));
                    }
                });
            });

    connect(ui->pushButtonExportKey, &QPushButton::clicked,
            this, [this, removeGroupBox]() {

                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setWindowTitle("Export Application Key");

                msgBox.setText(
                    "<div style='line-height:135%; margin:4px 0 6px 0;'>"
                    "<b><span style='color:#8c0000;'>This is a one‑time export.</span></b>"
                    "<div style='margin-top:8px;'>"
                    "Once you export the application key, it will be removed from the local database."
                    "<br><br>"
                    "<b>You will be required to provide this key manually every time the application starts.</b>"
                    "<br>"
                    "<ul style='margin:0; padding-left:20px;'>"
                    "<li style='margin-bottom:6px;'>If you lose this key, your stored data cannot be recovered.</li>"
                    "<li style='margin-bottom:6px;'>There is no way to regenerate the key.</li>"
                    "<li style='margin-bottom:6px;'>Keep the exported key in a safe, secure location away from the database.</li>"
                    "</ul>"
                    "</div>"
                    "<div style='margin-top:10px;'><b>Do you want to continue?</b></div>"
                    "</div>"
                    );

                msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
                msgBox.setDefaultButton(QMessageBox::Cancel);

                if (msgBox.exec() != QMessageBox::Ok)
                    return;

                // Retrieve the key
                const QString appKey =
                    qCompress(qApp->property("appKey").toString().toUtf8()).toBase64();

                // Build the message box that displays the key
                QMessageBox keyBox;
                keyBox.setIcon(QMessageBox::Information);
                keyBox.setWindowTitle("Your Application Key");

                // Make the box narrower by wrapping content in a fixed-width div
                QString html =
                    "<div style='width:420px; line-height:135%; margin:4px 0 6px 0;'>"
                    "<b><span style='color:#004a7f;'>This is the only time your application key will be shown.</span></b>"
                    "<div style='margin-top:8px;'>"
                    "Copy and store this key in a safe, secure location. "
                    "You will need to provide it manually every time the application starts."
                    "<br><br>"
                    "<b>If you lose this key, your stored data cannot be recovered.</b>"
                    "<br><br>"
                    "<div style='margin-top:10px; padding:8px; border:1px solid #ccc; "
                    "background:#f7f7f7; font-family:monospace; font-size:14px;'>"
                    + appKey +
                    "</div>"
                    "<div style='margin-top:12px;'>"
                    "Click <b>“Understood, I have a copy”</b> only after you have safely copied the key. "
                    "Click <b>Cancel</b> to abort without making any changes."
                    "</div>"
                    "</div>"
                    "</div>";

                keyBox.setText(html);

                QPushButton *understoodBtn = keyBox.addButton(
                    "Understood, I have a copy", QMessageBox::AcceptRole);

                keyBox.addButton(QMessageBox::Cancel);
                keyBox.setDefaultButton(QMessageBox::Cancel);
                keyBox.exec();

                if (keyBox.clickedButton() != understoodBtn)
                    return;

                const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

                {
                    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
                    db.setDatabaseName(qApp->property("dbFile").toString());

                    if (!db.open()) {
                        showDbNotOpenError(this, db, Q_FUNC_INFO);
                    } else {
                        QSqlQuery query(db);
                        query.prepare("UPDATE app_info SET value = 'exported' WHERE key ='app_key'");
                        if (query.exec())
                        {
                            removeGroupBox();
                            QMessageBox::information(
                                this,
                                QCoreApplication::applicationName(),
                                "The Application Key has been removed.\nChanges will take effect next restart."
                                );
                        } else
                        {
                            showQueryError(this, query, Q_FUNC_INFO);
                        }
                    }
                }

                QSqlDatabase::removeDatabase(connectionName);
            });

    restartRequired = false;
}

void PreferencesDialog::restoreButtonClicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole) {
        restoreDefaults();
    }
}

void PreferencesDialog::restoreDefaults()
{
    QMessageBox::StandardButton reply =
        QMessageBox::question(
            this,
            tr("Restore Defaults"),
            tr("This will reset all application preferences, including visual and interface settings, back to their default values.\n\n"
               "This does not restore or regenerate your Application Key if it has been previously exported.\n\n"
               "Do you want to continue?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );


    if (reply != QMessageBox::Yes) {
        return;
    }

    const QString targetPath = settings.configFilePath();
    QFile stock(":/files/passwords.conf");
    bool success = false;

    if (!stock.exists()) {
        qCritical().noquote() << Q_FUNC_INFO << "The boilerplate configuration file was not found inside the resource file.";
        QMessageBox::critical(this, tr("Restore Defaults"),
                              tr("The boilerplate configuration file was not found."));
        return;
    }

    if (!stock.open(QIODevice::ReadOnly)) {
        qCritical().noquote() << Q_FUNC_INFO << "The boilerplate configuration file could not be opened for reading.";
        QMessageBox::critical(this, tr("Restore Defaults"),
                              tr("Could not open the boilerplate configuration file."));
        return;
    }

    QByteArray data = stock.readAll();
    stock.close();

    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical().noquote() << Q_FUNC_INFO << "Could not write to" << targetPath << target.errorString();
        QMessageBox::critical(this, tr("Restore Defaults"),
                              tr("Could not write to %1").arg(targetPath));
        return;
    }

    if (target.write(data) != data.size()) {
        qCritical().noquote() << Q_FUNC_INFO << "Failed to write all data to" << targetPath << target.errorString();
        QMessageBox::critical(this, tr("Restore Defaults"),
                              tr("Failed to write all data to %1").arg(targetPath));
        target.close();
        return;
    }

    target.close();
    success = true;

    if (success) {
        loadSettings();
        qApp->setProperty("skipSave", true);
        QMessageBox::information(this, tr("Restore Defaults"),
                                 tr("Settings have been restored to defaults.\n\n"
                                    "Please restart the application for all default settings to take effect."));
        qInfo().noquote() << Q_FUNC_INFO << "Default configuration restored.";
    }
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::openBackupDir()
{
    QString settingsPath = settings.getDefaultDbPath(this);
    if (settingsPath.isEmpty()) {
        qCritical().noquote() << Q_FUNC_INFO << "Default DB Path is invalid.";
        QMessageBox::warning(this, tr("Error"), tr("Settings path is invalid."));
        return;
    }

    QFileInfo fi(settingsPath);
    QString dir = fi.absolutePath();
    QString backupsPath = QDir(dir).filePath("backups");

    if (!QDir().mkpath(backupsPath)) {
        qCritical().noquote() << Q_FUNC_INFO << "Could not create backups folder at" << backupsPath;
        QMessageBox::warning(this, tr("Error"), tr("Could not create backups folder."));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(backupsPath))) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open backups folder."));
    }
}

void PreferencesDialog::loadSettings()
{
    QSettings s(settings.configFilePath(), QSettings::IniFormat);

    for (auto it = widgetMap.begin(); it != widgetMap.end(); ++it) {
        const QString &key = it.key();
        QWidget *w = it.value();
        const QVariant val = s.value(key);

        if (auto edit = qobject_cast<QLineEdit*>(w)) {
            edit->setText(val.toString());

        } else if (auto check = qobject_cast<QCheckBox*>(w)) {
            check->setChecked(parseBool(val, check->isChecked()));

        } else if (auto group = qobject_cast<QGroupBox*>(w)) {
            // NEW: support checkable group boxes
            group->setChecked(parseBool(val, group->isChecked()));

        } else if (auto spin = qobject_cast<QSpinBox*>(w)) {
            spin->setValue(val.isValid() ? val.toInt() : spin->value());

        } else if (auto combo = qobject_cast<QComboBox*>(w)) {
            if (key == "Main/EchoMode") {
                int savedVal = val.isValid() ? val.toInt() : QLineEdit::Password;
                int idx = combo->findData(savedVal);
                if (idx >= 0)
                    combo->setCurrentIndex(idx);
            } else {
                const QString savedText = val.toString();
                int idx = combo->findText(savedText);
                if (idx >= 0)
                    combo->setCurrentIndex(idx);
            }
        }
    }

    ui->pushButton->setEnabled(ui->groupBox->isChecked());
}


void PreferencesDialog::saveSettings()
{
    qDebug() << "Saving to" << settings.configFilePath();
    QSettings s(settings.configFilePath(), QSettings::IniFormat);

    // Clear flag for this save operation
    restartRequired = false;

    for (auto it = widgetMap.begin(); it != widgetMap.end(); ++it) {
        const QString &key = it.key();
        QWidget *w = it.value();

        QVariant oldValue = s.value(key);
        QVariant newValue;

        if (auto edit = qobject_cast<QLineEdit*>(w)) {
            newValue = edit->text();
            s.setValue(key, newValue);

        } else if (auto check = qobject_cast<QCheckBox*>(w)) {
            newValue = check->isChecked();
            s.setValue(key, newValue);

        } else if (auto group = qobject_cast<QGroupBox*>(w)) {
            // support checkable group boxes
            newValue = group->isChecked();
            s.setValue(key, newValue);

        } else if (auto spin = qobject_cast<QSpinBox*>(w)) {
            newValue = spin->value();
            s.setValue(key, newValue);

        } else if (auto combo = qobject_cast<QComboBox*>(w)) {
            if (key == "Main/EchoMode") {
                newValue = combo->currentData().toInt();
            } else {
                newValue = combo->currentText();
            }
            s.setValue(key, newValue);
        }

        // Check if this setting requires restart and has actually changed
        if (restartKeys.contains(key) && oldValue != newValue) {
            restartRequired = true;
        }
    }

    // If backups disabled, remove backup folder (your existing logic)
    if (!ui->groupBox->isChecked()) {
        const QString dbPath = settings.getDefaultDbPath(this);
        if (!dbPath.isEmpty()) {
            const QString dbDir = QFileInfo(dbPath).absolutePath();
            const QString backupsPath = QDir(dbDir).filePath("backups");

            QDir dir(backupsPath);
            if (dir.exists()) {
                if (!dir.removeRecursively()) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Could not remove backups folder."));
                }
            }
        }
    }

    // Show restart message if needed
    if (restartRequired) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(tr("Restart Required"));
        msgBox.setText(
            tr("Some changes will take effect after restarting %1.")
                .arg(QApplication::applicationName())
            );

        QPushButton *restartBtn = msgBox.addButton(
            tr("Restart Now"), QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Ok);

        msgBox.exec();

        if (msgBox.clickedButton() == restartBtn) {
            restartApplication();
        }
    }

}

void PreferencesDialog::restartApplication()
{
    QString program = QCoreApplication::applicationFilePath();
    QStringList args = QCoreApplication::arguments();

    if (!args.isEmpty())
        args.removeFirst();

    // Tell MainWindow to skip quit confirmation
    if (auto mw = qobject_cast<MainWindow*>(parentWidget())) {
        mw->abortingStartup = true; //ignore the name, this one just lets us skip quit confirmation if configured in passwords.conf
    }

    // Start the new instance *right now*
    bool ok = QProcess::startDetached(program, args);

    if (!ok) {
        QMessageBox::critical(this, "Restart Failed",
                              "Could not restart the application.");
        return;
    }

    QCoreApplication::quit();
}

void PreferencesDialog::onBackupCheckStateChanged(int state)
{
    if (state == Qt::Unchecked) {
        // Ask the user before removing backups
        QMessageBox::StandardButton reply =
            QMessageBox::warning(this,
                                 tr("Remove Backups"),
                                 tr("Unchecking this option will delete the backups folder and all its contents.\n\n"
                                    "Are you sure you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            const QString dbPath = settings.getDefaultDbPath(this);
            if (!dbPath.isEmpty()) {
                const QString dbDir = QFileInfo(dbPath).absolutePath();
                const QString backupsPath = QDir(dbDir).filePath("backups");

                QDir dir(backupsPath);
                if (dir.exists() && !dir.removeRecursively()) {
                    qWarning().noquote() << Q_FUNC_INFO << "Could not remove the backups folder" << backupsPath;
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Could not remove backups folder."));
                }
            }
        } else {
            // User cancelled → restore the checkbox state
            ui->groupBox->setChecked(true);
        }
    }
}
