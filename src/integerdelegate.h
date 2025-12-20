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
