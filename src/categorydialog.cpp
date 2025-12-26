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


#include "categorydialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

CategoryDialog::CategoryDialog(QWidget *parent, int existingCount)
    : QDialog(parent)
{
    setWindowTitle(tr("Category"));

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(tr("Create a new category:"), this));

    m_nameEdit = new QLineEdit(this);
    layout->addWidget(m_nameEdit);

    m_topLevelCheck = new QCheckBox(tr("Top level item"), this);
    layout->addWidget(m_topLevelCheck);

    if (existingCount == 0) {
        m_topLevelCheck->setChecked(true);
        m_topLevelCheck->setDisabled(true);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString CategoryDialog::categoryName() const
{
    return m_nameEdit->text().trimmed();
}

bool CategoryDialog::isTopLevel() const
{
    return m_topLevelCheck->isChecked();
}
