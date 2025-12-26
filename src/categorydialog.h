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


// categorydialog.h
#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QVBoxLayout>

class CategoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CategoryDialog(QWidget *parent = nullptr, int existingCount = 0);

    QString categoryName() const;
    bool isTopLevel() const;

private:
    QLineEdit *m_nameEdit;
    QCheckBox *m_topLevelCheck;
};
