#ifndef WATERMARKEDTREEWIDGET_H
#define WATERMARKEDTREEWIDGET_H

#include <QTreeWidget>
#include <QDropEvent>
#include <QMimeData>

/**
 * A custom QTreeWidget that:
 *  - paints a watermark when empty
 *  - supports drag and drop
 *  - emits a signal when an item is dropped so MainWindow can handle the logic
 */
class WatermarkedTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit WatermarkedTreeWidget(QWidget *parent = nullptr);

    // Set or get the watermark text
    void setWatermarkText(const QString &text);
    QString watermarkText() const;

signals:
    // Emitted when a drop occurs; MainWindow can connect to this
    void itemDropped(QTreeWidgetItem *sourceItem, QTreeWidgetItem *targetItem);


protected:

    void paintEvent(QPaintEvent *event) override;
    QMimeData* mimeData(const QList<QTreeWidgetItem*> &items) const override;
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

private:
    QString m_watermarkText;
};

#endif // WATERMARKEDTREEWIDGET_H
