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


#include "aboutdialog.h"
#include "constants.h"
#include "gitversion.h"

#include <QCoreApplication>
#include <QFont>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QVBoxLayout>


AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About %1").arg(QCoreApplication::applicationName()));
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QVBoxLayout *main = new QVBoxLayout(this);

    main->addWidget(createIcon(this));
    main->addWidget(createAppName(this));
    main->addWidget(createVersion(this));
    main->addWidget(createCopyright(this));
    main->addWidget(createWebsite(this));

    // Credits (static, no collapsible section)
    main->addLayout(createCredits(this));

    resize(400, 426);
    setFixedSize(size());
}

// Icon with fade‑in animation
QWidget* AboutDialog::createIcon(QWidget *parent)
{
    QLabel *icon = new QLabel(parent);
    QPixmap pix(":/password.png");
    icon->setPixmap(pix.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);

    auto *opacity = new QGraphicsOpacityEffect(icon);
    icon->setGraphicsEffect(opacity);

    auto *fade = new QPropertyAnimation(opacity, "opacity");
    fade->setDuration(1000);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::InOutQuad);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    return icon;
}

// App name
QWidget* AboutDialog::createAppName(QWidget *parent)
{
    QLabel *label = new QLabel(QCoreApplication::applicationName(), parent);
    QFont f; f.setPointSize(14); f.setBold(true);
    label->setFont(f);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

// Version + git metadata
QWidget* AboutDialog::createVersion(QWidget *parent)
{
    QString version = QCoreApplication::applicationVersion();

#ifdef APP_DEBUG_BUILD
    version += "-debug";
#endif
#ifdef APP_RELEASE_BUILD
    version += "-release";
#endif

    QStringList git;
    if (!QString(GIT_COMMIT_HASH).isEmpty()) git << GIT_COMMIT_HASH;
    if (!QString(GIT_BRANCH).isEmpty())      git << GIT_BRANCH;

    // Build the base git string: commit.branch
    QString gitString;
    if (!git.isEmpty())
        gitString = git.join(".");

    // Append dirty suffix WITHOUT a dot
    if (!QString(GIT_DIRTY).isEmpty())
        gitString += GIT_DIRTY;   // "-dirty" or ""

    // Add to version
    if (!gitString.isEmpty())
        version += "+" + gitString;

    version += QString(" (%1)").arg(BUILD_TIMESTAMP);

    QLabel *label = new QLabel(version, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    return label;
}


// Copyright
QWidget* AboutDialog::createCopyright(QWidget *parent)
{
    QLabel *label = new QLabel(
        tr("Copyright © 2026 %1.<br/>")
            .arg(QStringLiteral("Adam Lanzafame")),
        parent);

    QFont f; f.setPointSize(8);
    label->setFont(f);
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    return label;
}

// Website link
QWidget* AboutDialog::createWebsite(QWidget *parent)
{
    QLabel *label = new QLabel(
        tr("Website: %1")
            .arg(QString("<a href=\"%1\">%1</a>").arg(Passwords::GitUrl)),
        parent
        );

    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    return label;
}

// Credits section
QLayout* AboutDialog::createCredits(QWidget *parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(6);
    layout->setContentsMargins(0,0,0,0);

    QFont small;
    small.setPointSize(8);

    auto makePlain = [&](const QString &text) {
        QLabel *l = new QLabel(text, parent);
        l->setAlignment(Qt::AlignCenter);
        l->setFont(small);
        l->setWordWrap(true);
        l->setTextFormat(Qt::PlainText);
        return l;
    };

    auto makeLink = [&](const QString &html) {
        QLabel *l = new QLabel(html, parent);
        l->setAlignment(Qt::AlignCenter);
        l->setFont(small);
        l->setTextFormat(Qt::RichText);
        l->setOpenExternalLinks(true);
        return l;
    };

    // Required attributions
    layout->addWidget(makePlain(Passwords::WordlistsCredit));
    layout->addWidget(makePlain(Passwords::MaterialSymbolsCredit));
    layout->addWidget(makePlain(QString(Passwords::IconCreditFormat).arg(QCoreApplication::applicationName())));
    layout->addWidget(makePlain(Passwords::EmojiCredit));

    layout->addSpacing(4);

    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    layout->addSpacing(4);

    // GPL notice (friendly wording + clickable link)
    layout->addWidget(makeLink(Passwords::License));

    layout->addWidget(makePlain(Passwords::WarrantyDisclaimer));

    return layout;
}
