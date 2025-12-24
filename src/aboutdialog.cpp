#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QApplication>
#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFrame>
#include <QFont>

// Build metadata
#include "gitversion.h"

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

    resize(400, 440);
    setFixedSize(size());
}

// ------------------------------------------------------------
// Icon with fade‑in animation
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// App name
// ------------------------------------------------------------
QWidget* AboutDialog::createAppName(QWidget *parent)
{
    QLabel *label = new QLabel(QCoreApplication::applicationName(), parent);
    QFont f; f.setPointSize(14); f.setBold(true);
    label->setFont(f);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

// ------------------------------------------------------------
// Version + git metadata
// ------------------------------------------------------------
QWidget* AboutDialog::createVersion(QWidget *parent)
{
    QString version = QApplication::applicationVersion();

#ifdef APP_DEBUG_BUILD
    version += "-debug";
#endif
#ifdef APP_RELEASE_BUILD
    version += "-release";
#endif

    QStringList git;
    if (!QString(GIT_COMMIT_HASH).isEmpty()) git << GIT_COMMIT_HASH;
    if (!QString(GIT_BRANCH).isEmpty())      git << GIT_BRANCH;
    if (!QString(GIT_DIRTY).isEmpty())       git << GIT_DIRTY;

    if (!git.isEmpty())
        version += "+" + git.join(".");

    version += QString(" (%1)").arg(BUILD_TIMESTAMP);

    QLabel *label = new QLabel(version, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    return label;
}

// ------------------------------------------------------------
// Copyright
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Website link
// ------------------------------------------------------------
QWidget* AboutDialog::createWebsite(QWidget *parent)
{
    QLabel *label = new QLabel(
        tr("Website: %1")
            .arg("<a href=\"https://github.com/sysal1280/passwords\">https://github.com/sysal1280/passwords</a>"),
        parent);

    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    return label;
}

// ------------------------------------------------------------
// Credits section
// ------------------------------------------------------------
QLayout* AboutDialog::createCredits(QWidget *parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(6);
    layout->setContentsMargins(0,0,0,0);

    QFont small; small.setPointSize(8);

    auto make = [&](const QString &html) {
        QLabel *l = new QLabel(html, parent);
        l->setAlignment(Qt::AlignCenter);
        l->setFont(small);
        l->setTextFormat(Qt::RichText);
        l->setOpenExternalLinks(true);
        l->setTextInteractionFlags(Qt::TextBrowserInteraction);
        return l;
    };

    layout->addWidget(make(tr("Wordlists from Orchard Street Wordlists by %1.")
                               .arg("<a href=\"https://www.samschlinkert.com/\">Sam Schlinkert</a>")));

    layout->addWidget(make(tr("Material Symbols from Google Fonts<br/>"
                              "Licensed under the %1, Version 2.0.")
                               .arg("<a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License</a>")));

    layout->addWidget(make(tr("Passwords icon created by %1.")
                               .arg("<a href=\"https://www.flaticon.com/free-icons/password\">Iconic Panda - Flaticon</a>")));

    layout->addSpacing(4);

    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    layout->addSpacing(4);

    layout->addWidget(make(tr("This program comes with absolutely no warranty.<br>"
                              "See the %1 or later for details.")
                               .arg("<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU GPLv3</a>")));

    QLabel *gpl = new QLabel(parent);
    QPixmap logo(":/pngs/gplv3-with-text-136x68.png");
    gpl->setPixmap(logo.scaledToWidth(136, Qt::SmoothTransformation));
    gpl->setAlignment(Qt::AlignCenter);
    layout->addWidget(gpl);

    return layout;
}
