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

#include "watermarkedtreewidget.h"

#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QDrag>

WatermarkedTreeWidget::WatermarkedTreeWidget(QWidget *parent)
    : QTreeWidget(parent), m_watermarkText("Default Watermark") {
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::CopyAction);   // <-- REQUIRED
    setDropIndicatorShown(true);
}


void WatermarkedTreeWidget::setWatermarkText(const QString &text) {
    m_watermarkText = text;
    viewport()->update();
}

QString WatermarkedTreeWidget::watermarkText() const {
    return m_watermarkText;
}

void WatermarkedTreeWidget::paintEvent(QPaintEvent *event) {
    QTreeWidget::paintEvent(event);

    if (topLevelItemCount() == 0 && !m_watermarkText.isEmpty()) {
        QPainter painter(viewport());
        QFont font = painter.font();
        font.setPointSize(24);
        font.setBold(true);
        painter.setFont(font);

        painter.setPen(QColor(200, 200, 200, 100));

        QRect fullRect = viewport()->rect();

        // Draw text centered in the full widget
        painter.drawText(fullRect, Qt::AlignCenter, m_watermarkText);
    }
}

void WatermarkedTreeWidget::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        event->ignore();
        return;
    }

    QByteArray ba = event->mimeData()->data("application/x-qtreewidgetitem");
    QString data = QString::fromUtf8(ba);

    QStringList itemBlocks = data.split(":::");
    QList<QTreeWidgetItem*> droppedItems;
    qDebug() << "Dropped blocks:" << itemBlocks;


    for (int i = 0; i < itemBlocks.size(); ++i) {
        const QString &block = itemBlocks.at(i);
        QStringList cols = block.split(";");

        auto *item = new QTreeWidgetItem();
        for (int c = 0; c < cols.size(); ++c) {
            QStringList parts = cols[c].split("|");
            item->setText(c, parts.value(0));
            item->setData(c, Qt::UserRole, parts.value(1));
        }

        droppedItems << item;
    }

    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());

    if (droppedItems.count() == 1) {
        emit itemDropped(droppedItems.first(), targetItem);
    } else {
        emit itemsDropped(droppedItems, targetItem);
    }

    event->acceptProposedAction();
}


QMimeData* WatermarkedTreeWidget::mimeData(const QList<QTreeWidgetItem*> &items) const
{
    QMimeData *mime = new QMimeData();

    if (items.isEmpty())
        return mime;

    QStringList serializedItems;

    for (QTreeWidgetItem *item : items) {
        QStringList cols;

        for (int c = 0; c < item->columnCount(); ++c) {
            QString text = item->text(c);
            QString role = item->data(c, Qt::UserRole).toString();
            cols << text + "|" + role;
        }

        serializedItems << cols.join(";");
    }

    // Items separated by ":::"
    mime->setData("application/x-qtreewidgetitem",
                  serializedItems.join(":::").toUtf8());

    return mime;
}


void WatermarkedTreeWidget::dragEnterEvent(QDragEnterEvent *event) {
    qDebug() << "Formats:" << event->mimeData()->formats();

    if (event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void WatermarkedTreeWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void WatermarkedTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QList<QTreeWidgetItem*> items = selectedItems();
    if (items.isEmpty())
        return;

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData(items));

    //
    // --- Drag Pixmap ---
    //

    QPixmap pm(140, 40);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bg(34, 153, 212, 180);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(pm.rect(), 10, 10);

    p.setPen(Qt::white);
    QFont font;
    font.setBold(true);
    font.setPointSize(11);
    p.setFont(font);

    QString label;
    if (items.count() == 1)
        label = items.first()->text(0);
    else
        label = QString("%1 passwords").arg(items.count());

    p.drawText(pm.rect(), Qt::AlignCenter, label);
    p.end();

    drag->setPixmap(pm);
    drag->setHotSpot(QPoint(pm.width() / 2, pm.height() / 2));

    drag->exec(Qt::CopyAction);
}
