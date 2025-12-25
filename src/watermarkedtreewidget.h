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
