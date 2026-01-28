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


#ifndef MULTILINEDELEGATE_H
#define MULTILINEDELEGATE_H

#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>

class MultiLineDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit MultiLineDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &) const override {
        QTextEdit *editor = new QTextEdit(parent);
        editor->setAcceptRichText(false);
        editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        const int maxBytes = 16 * 1024; // 64 KB

        connect(editor, &QTextEdit::textChanged, editor, [editor, maxBytes]() {
            QByteArray utf8 = editor->toPlainText().toUtf8();
            if (utf8.size() > maxBytes) {
                utf8.truncate(maxBytes);

                editor->blockSignals(true);
                editor->setPlainText(QString::fromUtf8(utf8));
                editor->moveCursor(QTextCursor::End);
                editor->blockSignals(false);

                QTimer::singleShot(0, editor, [editor]() {
                    QMessageBox::warning(
                        editor->window(),
                        "Text Limit Reached",
                        "The maximum size for this field is 16 KB.\n"
                        "Your text has been truncated."
                        );
                });

            }
        });

        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        static_cast<QTextEdit*>(editor)->setPlainText(value);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        QTextEdit *edit = static_cast<QTextEdit*>(editor);
        QByteArray utf8 = edit->toPlainText().toUtf8();

        const int maxBytes = 16 * 1024;

        if (utf8.size() > maxBytes)
            utf8.truncate(maxBytes);

        model->setData(index, QString::fromUtf8(utf8), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &) const override {
        editor->setGeometry(option.rect);
    }
};

#endif // MULTILINEDELEGATE_H
