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


#ifndef NEWPASSWORDDIALOG_H
#define NEWPASSWORDDIALOG_H

#include "keyentry.h"
#include "settings.h"

#include <QDialog>
#include <QList>

namespace Ui {
class NewPasswordDialog;
}

class NewPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewPasswordDialog(QWidget *parent = nullptr);
    ~NewPasswordDialog();

    QByteArray toJson() const;
    void setKeys(const QList<KeyEntry> &keys);
    QStringList getAllKeys() const;
    QStringList getCheckedKeys() const;
    QString URL;
    QString Description;
    QString AppName;
    QString PublicAppName;

    void openPassword();
    void openCredentials(QString username, QString password, QString secretOptCode, int length);
    void openNote(QString note);

private slots:
    void onNotesContextMenu(const QPoint &pos);
    void onCredentialsContextMenu(const QPoint &pos);
    void validateForm();

private:
    Ui::NewPasswordDialog *ui;
    QList<KeyEntry> m_keys;
    void suggestFields();
    Settings settings;
};

#endif // NEWPASSWORDDIALOG_H
