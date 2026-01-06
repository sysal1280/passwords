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


#include "droplabel.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

DropLabel::DropLabel(QWidget *parent)
    : QLabel(parent)
{
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);

    // attach opacity effect
    auto *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(1.0);  // ensure full opacity from the star
    setGraphicsEffect(effect);
}

void DropLabel::dragEnterEvent(QDragEnterEvent *event)
{
    // Only accept our custom MIME type
    if (!event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        event->ignore();
        return;
    }

    // Extract payload
    const QByteArray ba = event->mimeData()->data("application/x-qtreewidgetitem");
    const QString payload = QString::fromUtf8(ba);

    // Reject multi-item drags (they contain ":::")
    if (payload.contains(":::")) {
        event->ignore();
        return;
    }

    // Single item → accept
    event->acceptProposedAction();
    fadeToPixmap(QPixmap(":/place_alive.png"));
}


void DropLabel::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-qtreewidgetitem")) {
        event->ignore();
        return;
    }

    const QByteArray ba = event->mimeData()->data("application/x-qtreewidgetitem");
    const QString payload = QString::fromUtf8(ba);

    // Expect columns separated by \t, each column as "text|userRoleString"
    const QStringList cols = payload.split("\t", Qt::KeepEmptyParts);

    auto *item = new QTreeWidgetItem();
    for (int c = 0; c < cols.size(); ++c) {
        const QStringList parts = cols[c].split("|", Qt::KeepEmptyParts);
        const QString text = parts.value(0);
        const QString userRoleStr = parts.value(1); // may be empty

        item->setText(c, text);

        // Store user role as-is (string) and also try to interpret as int if appropriate
        if (!userRoleStr.isEmpty()) {
            // Preserve original string role
            item->setData(c, Qt::UserRole, userRoleStr);

            // If it looks like an integer, also store as int in UserRole + 1 (optional)
            bool ok = false;
            int asInt = userRoleStr.toInt(&ok);
            if (ok) {
                item->setData(c, Qt::UserRole + 1, asInt);
            }
        }
    }

    emit itemDropped(item);
    event->acceptProposedAction();
    fadeToPixmap(QPixmap(":/place.png"));
}

void DropLabel::dragLeaveEvent(QDragLeaveEvent *event)
{
    QLabel::dragLeaveEvent(event);
    fadeToPixmap(QPixmap(":/place.png"));
}

void DropLabel::fadeToPixmap(const QPixmap &pixmap)
{
    auto *effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
    if (!effect) return;

    // fade out
    auto *animOut = new QPropertyAnimation(effect, "opacity");
    animOut->setDuration(250);
    animOut->setStartValue(1.0);
    animOut->setEndValue(0.0);

    connect(animOut, &QPropertyAnimation::finished, this, [this, pixmap, effect]() {
        setPixmap(pixmap);

        // fade back in
        auto *animIn = new QPropertyAnimation(effect, "opacity");
        animIn->setDuration(200);
        animIn->setStartValue(0.0);
        animIn->setEndValue(1.0);
        animIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    animOut->start(QAbstractAnimation::DeleteWhenStopped);
}
