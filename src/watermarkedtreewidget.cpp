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

WatermarkedTreeWidget::WatermarkedTreeWidget(QWidget *parent)
    : QTreeWidget(parent), m_watermarkText("Default Watermark") {
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
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

void WatermarkedTreeWidget::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        QByteArray ba = event->mimeData()->data("application/x-qtreewidgetitem");
        QStringList cols = QString::fromUtf8(ba).split("\t");

        QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());

        auto *sourceItem = new QTreeWidgetItem();
        for (int c = 0; c < cols.size(); ++c) {
            QStringList parts = cols[c].split("|");
            sourceItem->setText(c, parts.value(0));
            if (parts.size() > 1)
                sourceItem->setData(c, Qt::UserRole, parts.value(1));
        }

        emit itemDropped(sourceItem, targetItem);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

QMimeData* WatermarkedTreeWidget::mimeData(const QList<QTreeWidgetItem*> &items) const {
    QMimeData *mime = new QMimeData();
    if (!items.isEmpty()) {
        QStringList cols;
        for (int c = 0; c < items.first()->columnCount(); ++c) {
            QString text = items.first()->text(c);
            QVariant roleData = items.first()->data(c, Qt::UserRole);
            // Store text and role data together, separated by '|'
            cols << text + "|" + roleData.toString();
        }
        mime->setData("application/x-qtreewidgetitem", cols.join("\t").toUtf8());
    }
    return mime;
}

void WatermarkedTreeWidget::dragEnterEvent(QDragEnterEvent *event) {
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
