/*
 * passwords - A simple password manager
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
#include "mainwindow.h"
#include "settings.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QResizeEvent>
#include <QAbstractButton>
#include <QDebug>
#include <QResource>

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

    /*
     * Setup tab labels
     */
    QStringList labels = { tr("General"), tr("Passwords"), tr("Bookmarks"), tr("Help")  };
    for (int i = 0; i < labels.size(); ++i) {
        ui->tabWidget->setTabText(i, labels.at(i));
    }

    ui->tabWidget->setCurrentIndex(0);

    /*
     * Load wordlist rc file.
     */

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString wordListPath = QDir(configDir).filePath("wordlist.rc");

    if (QFile::exists(wordListPath)) {
        if (QResource::registerResource(wordListPath)) {
            qDebug() << "Loaded wordlist.rc";
        } else {
            qCritical() << "Failed to register resource:" << wordListPath;
            QMessageBox::critical(this,"",QString(tr("Failed to register resource: %1")).arg(wordListPath));
        }
    } else
    {
        qCritical() << "Missing resource file:" << wordListPath;
        QMessageBox::critical(this,"",QString(tr("Missing resource file: %1")).arg(wordListPath));
    }

    QDir dir(":/wordlist");
    QStringList entries = dir.entryList(QDir::Files);
    for (const QString &name : std::as_const(entries)) {
        ui->comboBoxWordList->addItem(name);
    }
    QResource::unregisterResource(wordListPath);

    ui->labelOrchidStreetWords->setOpenExternalLinks(true);


    /*
     * Build the settings map
     */

    widgetMap.insert("General/BackupDatabase", ui->checkBoxBackupDB);
    widgetMap.insert("General/AskBeforeClosing", ui->checkBoxAskClose);
    widgetMap.insert("General/RequireChallenge", ui->checkBoxRequireChallenge);
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


    /*
     * Range settings for spinboxes
     */
    ui->spinBox->setRange(2,10);
    ui->spinBoxAutoClose->setRange(0,600);

    loadSettings();
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &PreferencesDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &PreferencesDialog::restoreButtonClicked);
    connect(ui->checkBoxBackupDB, &QCheckBox::toggled,
            ui->pushButton, &QPushButton::setEnabled);
    connect(ui->checkBoxBackupDB, &QCheckBox::checkStateChanged,
            this, &PreferencesDialog::onBackupCheckStateChanged);

}

void PreferencesDialog::restoreButtonClicked(QAbstractButton *button)
{
    /*
     * Restore button clicked
     */
    QDialogButtonBox::ButtonRole role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole) {
        restoreDefaults();
    }
}

void PreferencesDialog::restoreDefaults()
{

    QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("Restore Defaults"),
                              tr("This will overwrite your current settings including graphcal interface settings with the default configuration.\n\n"
                                 "Are you sure you want to continue?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return; // user cancelled
    }

    const QString targetPath = Settings::configFilePath();
    QFile stock(":/files/passwords.conf");
    bool success = false;

    if (!stock.exists()) {
        QMessageBox::critical(this, "",
                              tr("Stock configuration file not found in resources."));
        return;
    }

    if (!stock.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "",
                              tr("Could not open stock configuration file."));
        return;
    }

    QByteArray data = stock.readAll();
    stock.close();

    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "",
                              tr("Could not write to %1").arg(targetPath));
        return;
    }

    if (target.write(data) != data.size()) {
        QMessageBox::critical(this, "",
                              tr("Failed to write all data to %1").arg(targetPath));
        target.close();
        return;
    }

    target.close();
    success = true;

    if (success) {
        loadSettings();
        qApp->setProperty("skipSave", true);
        QMessageBox::information(this, "",
                                 tr("Settings have been restored to defaults.\n\n"
                                    "Please restart the application for all default settings to take effect."));
    }

}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_pushButton_clicked()
{
    QString settingsPath = Settings::getDefaultDbPath(this);
    if (settingsPath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Settings path is invalid."));
        return;
    }

    QFileInfo fi(settingsPath);
    QString dir = fi.absolutePath();
    QString backupsPath = QDir(dir).filePath("backups");

    if (!QDir().mkpath(backupsPath)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not create backups folder."));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(backupsPath))) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open backups folder."));
    }
}

void PreferencesDialog::loadSettings()
{
    QSettings s(Settings::configFilePath(), QSettings::IniFormat);

    for (auto it = widgetMap.begin(); it != widgetMap.end(); ++it) {
        const QString &key = it.key();
        QWidget *w = it.value();
        const QVariant val = s.value(key);

        if (auto edit = qobject_cast<QLineEdit*>(w)) {
            edit->setText(val.toString());
        } else if (auto check = qobject_cast<QCheckBox*>(w)) {
            check->setChecked(parseBool(val, check->isChecked()));
        } else if (auto spin = qobject_cast<QSpinBox*>(w)) {
            spin->setValue(val.isValid() ? val.toInt() : spin->value());
        } else if (auto combo = qobject_cast<QComboBox*>(w)) {
            // Load by text instead of index
            const QString savedText = val.toString();
            int idx = combo->findText(savedText);
            if (idx >= 0)
                combo->setCurrentIndex(idx);
        }
    }

    ui->pushButton->setEnabled(ui->checkBoxBackupDB->isChecked());
}

void PreferencesDialog::saveSettings()
{
    QSettings s(Settings::configFilePath(), QSettings::IniFormat);

    for (auto it = widgetMap.begin(); it != widgetMap.end(); ++it) {
        QWidget *w = it.value();

        if (auto edit = qobject_cast<QLineEdit*>(w))
            s.setValue(it.key(), edit->text());
        else if (auto check = qobject_cast<QCheckBox*>(w))
            s.setValue(it.key(), check->isChecked());
        else if (auto spin = qobject_cast<QSpinBox*>(w))
            s.setValue(it.key(), spin->value());
        else if (auto combo = qobject_cast<QComboBox*>(w))
            // Save the text instead of the index
            s.setValue(it.key(), combo->currentText());
    }

    if (!ui->checkBoxBackupDB->isChecked())
    {
        const QString dbPath = Settings::getDefaultDbPath(this);
        if (!dbPath.isEmpty()) {
            const QString dbDir = QFileInfo(dbPath).absolutePath();
            const QString backupsPath = QDir(dbDir).filePath("backups");

            QDir dir(backupsPath);
            if (dir.exists()) {
                if (!dir.removeRecursively()) {
                    QMessageBox::warning(this, tr("Error"), tr("Could not remove backups folder."));
                }
            }
        }
    }

}

void PreferencesDialog::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    setWindowState(Qt::WindowNoState);
    adjustSize();
    setFixedSize(size()); // snap back to fixed size on any resize
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
            const QString dbPath = Settings::getDefaultDbPath(this);
            if (!dbPath.isEmpty()) {
                const QString dbDir = QFileInfo(dbPath).absolutePath();
                const QString backupsPath = QDir(dbDir).filePath("backups");

                QDir dir(backupsPath);
                if (dir.exists() && !dir.removeRecursively()) {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Could not remove backups folder."));
                }
            }
        } else {
            // User cancelled → restore the checkbox state
            ui->checkBoxBackupDB->setChecked(true);
        }
    }
}

void PreferencesDialog::on_buttonBox_helpRequested()
{
    if (auto mw = qobject_cast<MainWindow*>(parentWidget())) {
        mw->launchHelperProcess("preferences");
    }
}

