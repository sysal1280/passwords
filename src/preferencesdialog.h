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


#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "settings.h"

#include <QAbstractButton>
#include <QDialog>
#include <QMap>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();
    void loadSettings();
    void saveSettings();

private slots:

private:
    Ui::PreferencesDialog *ui;
    QMap<QString, QWidget*> widgetMap;
    void resizeEvent(QResizeEvent *event);
    void restoreButtonClicked(QAbstractButton *button);
    void restoreDefaults();
    void onBackupCheckStateChanged(int state);
    void openBackupDir();
    void restartApplication();
    Settings settings;

    bool restartRequired = false;
    QSet<QString> restartKeys = {
        "Categories/DoubleClickOpen"   // setting which applied net restart
    };


};

#endif // PREFERENCESDIALOG_H
