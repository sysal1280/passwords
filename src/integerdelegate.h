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


#ifndef INTEGERDELEGATE_H
#define INTEGERDELEGATE_H

#include <QStyledItemDelegate>
#include <QSpinBox>

class IntegerDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit IntegerDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &) const override {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(0);      // adjust as needed
        editor->setMaximum(999999); // adjust as needed
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        int value = index.model()->data(index, Qt::EditRole).toInt();
        static_cast<QSpinBox*>(editor)->setValue(value);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        int value = static_cast<QSpinBox*>(editor)->value();
        model->setData(index, value, Qt::EditRole);
    }
};

#endif // INTEGERDELEGATE_H
