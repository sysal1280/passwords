#ifndef MULTILINEDELEGATE_H
#define MULTILINEDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextEdit>

class MultiLineDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit MultiLineDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &) const override {
        QTextEdit *editor = new QTextEdit(parent);
        editor->setAcceptRichText(false);   // plain text only
        editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        static_cast<QTextEdit*>(editor)->setPlainText(value);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        QString value = static_cast<QTextEdit*>(editor)->toPlainText();
        model->setData(index, value, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &) const override {
        editor->setGeometry(option.rect);
    }
};

#endif // MULTILINEDELEGATE_H
