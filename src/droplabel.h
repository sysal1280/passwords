#ifndef DROPLABEL_H
#define DROPLABEL_H

#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTreeWidgetItem>

class DropLabel : public QLabel {
    Q_OBJECT
public:
    explicit DropLabel(QWidget *parent = nullptr);

signals:
    void itemDropped(QTreeWidgetItem *item);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void fadeToPixmap(const QPixmap &pixmap);

};

#endif // DROPLABEL_H
