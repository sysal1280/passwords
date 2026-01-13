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


#include "passworddialog.h"
#include "passwordgenerator.h"
#include "settings.h"
#include "utils.h"


#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QResource>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QVBoxLayout>

#include <QtMath>


namespace PasswordDialog {

PasswordInspectorDialog::PasswordInspectorDialog(const QString &password,
                                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Password Inspector");
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *main = new QVBoxLayout(this);

    // Big password label
    QLabel *pwLabel = new QLabel(password, this);
    QFont big; big.setPointSize(18); big.setBold(true);
    pwLabel->setFont(big);
    pwLabel->setAlignment(Qt::AlignCenter);
    pwLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    pwLabel->setObjectName("labelPasswordInspector");
    pwLabel->setWordWrap(true);
    main->addWidget(pwLabel);

    // Scroll area for tiles
    QScrollArea *scroll = new QScrollArea(this);

    // We will control the child + height ourselves
    scroll->setWidgetResizable(false);

    // No vertical scrollbars, horizontal as needed
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // important for stable height

    QWidget *tilesContainer = new QWidget(scroll);
    QHBoxLayout *tiles = new QHBoxLayout(tilesContainer);
    tiles->setSpacing(8);
    tiles->setContentsMargins(0, 4, 0, 4);

    // Make tiles taller so they visually occupy more vertical space
    int maxTileHeight = 0;
    for (QChar c : password) {
        QWidget *tile = createCharacterTile(c);

        int base = tile->sizeHint().height();
        int h = static_cast<int>(base * 1.8) -16;   // take up more vertical space
        tile->setFixedHeight(h);

        tiles->addWidget(tile);
        maxTileHeight = std::max(maxTileHeight, h);
    }

    tiles->addStretch();

    // --- Compute exact heights so there is ZERO vertical scroll range ---

    const QMargins m = tiles->contentsMargins();
    const int topMargin    = m.top();
    const int bottomMargin = m.bottom();

    // Height of the row of tiles inside the container
    const int containerHeight = maxTileHeight + topMargin + bottomMargin;
    tilesContainer->setFixedHeight(containerHeight);

    scroll->setWidget(tilesContainer);

    // Frame + horizontal scrollbar height
    const int frame = scroll->frameWidth();
    const int hScrollExtent =
        scroll->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, scroll);

    // Viewport height must equal containerHeight, so scroll height is:
    const int scrollHeight = containerHeight + 2 * frame + hScrollExtent;
    scroll->setFixedHeight(scrollHeight);

    main->addWidget(scroll);

    // Legend aligned right
    QWidget *legend = new QWidget(this);
    QHBoxLayout *legendLayout = new QHBoxLayout(legend);
    legendLayout->setSpacing(15);
    legendLayout->setContentsMargins(0, 0, 0, 0);
    legendLayout->setAlignment(Qt::AlignRight);

    legendLayout->addWidget(makeLegendItem("#1976D2", "Uppercase"));
    legendLayout->addWidget(makeLegendItem("#388E3C", "Lowercase"));
    legendLayout->addWidget(makeLegendItem("#7B1FA2", "Number"));
    legendLayout->addWidget(makeLegendItem("#F57C00", "Symbol"));

    main->addWidget(legend);

    // Analysis section
    PasswordAnalysis analysis = analyzePassword(password);

    QLabel *analysisLabel = new QLabel(formatAnalysisSummary(analysis), this);
    analysisLabel->setWordWrap(true);
    analysisLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont analysisFont = analysisLabel->font();
    analysisFont.setPointSize(9);
    analysisLabel->setFont(analysisFont);

    main->addWidget(analysisLabel);

    // Let Qt compute correct height now that scroll area has a fixed, correct height
    adjustSize();
}

QWidget *PasswordInspectorDialog::makeLegendItem(const QString &color, const QString &label)
{
    QWidget *item = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    QLabel *square = new QLabel;
    square->setFixedSize(14, 14);
    square->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(color));

    QLabel *text = new QLabel(label);

    layout->addWidget(square);
    layout->addWidget(text);

    return item;
}

QWidget* PasswordInspectorDialog::createCharacterTile(QChar c)
{
    QFrame *frame = new QFrame;
    frame->setProperty("tileType", characterType(c));

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(8, 6, 8, 6);

    QLabel *charLabel = new QLabel(QString(c), frame);
    QFont big; big.setPointSize(18); big.setBold(true);
    charLabel->setFont(big);
    charLabel->setAlignment(Qt::AlignCenter);

    QLabel *descLabel = new QLabel(characterDescription(c), frame);
    QFont small; small.setPointSize(8);
    descLabel->setFont(small);
    descLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(charLabel);
    layout->addWidget(descLabel);

    return frame;
}

QString PasswordInspectorDialog::characterType(QChar c) const
{
    if (c.isUpper()) return "upper";
    if (c.isLower()) return "lower";
    if (c.isDigit()) return "digit";
    return "symbol";
}

QString PasswordInspectorDialog::symbolName(QChar c) const
{
    switch (c.unicode()) {
    case u'!': return "Exclamation mark";
    case u'"': return "Quotation mark";
    case u'#': return "Number sign";
    case u'$': return "Dollar sign";
    case u'%': return "Percent sign";
    case u'&': return "Ampersand";
    case u'\'': return "Apostrophe";
    case u'(': return "Left parenthesis";
    case u')': return "Right parenthesis";
    case u'*': return "Asterisk";
    case u'+': return "Plus sign";
    case u',': return "Comma";
    case u'-': return "Hyphen-minus";
    case u'.': return "Full stop";
    case u'/': return "Solidus";
    case u':': return "Colon";
    case u';': return "Semicolon";
    case u'<': return "Less-than sign";
    case u'=': return "Equals sign";
    case u'>': return "Greater-than sign";
    case u'?': return "Question mark";
    case u'@': return "Commercial at";
    case u'[': return "Left square bracket";
    case u'\\': return "Reverse solidus";
    case u']': return "Right square bracket";
    case u'^': return "Caret";
    case u'_': return "Underscore";
    case u'`': return "Grave accent";
    case u'{': return "Left curly bracket";
    case u'|': return "Vertical line";
    case u'}': return "Right curly bracket";
    case u'~': return "Tilde";
    case u' ': return "Space";
    default:
        return QString("Symbol (U+%1)").arg(QString::number(c.unicode(), 16).toUpper());
    }
}

QString PasswordInspectorDialog::characterDescription(QChar c) const
{
    if (c.isUpper()) return QString("Uppercase %1").arg(c);
    if (c.isLower()) return QString("Lowercase %1").arg(c);
    if (c.isDigit()) return QString("Number %1").arg(c);
    return symbolName(c);
}

PasswordInspectorDialog::PasswordAnalysis
PasswordInspectorDialog::analyzePassword(const QString &password) const
{
    PasswordAnalysis a;
    a.length = password.size();

    bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;

    for (QChar c : password) {
        if (c.isUpper()) { a.upperCount++; hasUpper = true; }
        else if (c.isLower()) { a.lowerCount++; hasLower = true; }
        else if (c.isDigit()) { a.digitCount++; hasDigit = true; }
        else { a.symbolCount++; hasSymbol = true; }
    }

    int charsetSize = 0;
    if (hasLower) charsetSize += 26;
    if (hasUpper) charsetSize += 26;
    if (hasDigit) charsetSize += 10;
    if (hasSymbol) charsetSize += 32;

    if (charsetSize > 0 && a.length > 0) {
        double log2charset = qLn(double(charsetSize)) / qLn(2.0);
        a.entropyBits = a.length * log2charset;
    }

    if (a.entropyBits < 40) a.entropyLabel = "Weak";
    else if (a.entropyBits < 60) a.entropyLabel = "Fair";
    else if (a.entropyBits < 80) a.entropyLabel = "Strong";
    else a.entropyLabel = "Very strong";

    a.segmentation = segmentPassword(password);

    return a;
}

QString PasswordInspectorDialog::segmentPassword(const QString &password) const
{
    if (password.isEmpty()) return QString();

    QStringList segments;
    QString current;
    auto isLetter = [](QChar ch) { return ch.isLetter(); };

    bool currentIsLetter = isLetter(password[0]);
    current.append(password[0]);

    for (int i = 1; i < password.size(); ++i) {
        QChar c = password[i];
        bool letter = isLetter(c);
        if (letter == currentIsLetter) {
            current.append(c);
        } else {
            segments << current;
            current.clear();
            currentIsLetter = letter;
            current.append(c);
        }
    }
    segments << current;

    return segments.join(" | ");
}

QString PasswordInspectorDialog::formatAnalysisSummary(const PasswordAnalysis &a) const
{
    QString summary;

    summary += QString("Entropy: %1 bits (%2)\n")
                   .arg(QString::number(a.entropyBits, 'f', 1),
                        a.entropyLabel);

    summary += QString("Length: %1 characters\n")
                   .arg(a.length);

    summary += QString("Character classes: %1 uppercase, %2 lowercase, %3 Numbers, %4 symbols\n")
                   .arg(a.upperCount)
                   .arg(a.lowerCount)
                   .arg(a.digitCount)
                   .arg(a.symbolCount);

    summary += QString("Segmentation: %1")
                   .arg(a.segmentation);

    return summary;
}

void showPasswordGenerator(QWidget *parent,
                           const QString &title,
                           const QStringList & /*wordList*/)
{
    Settings settings;

    QString wordListPath = loadWordlistResource(parent, Q_FUNC_INFO);

    QStringList wl = passwordGenerator::loadWordList(settings.getWordListFile());

    if (!QResource::unregisterResource(wordListPath))
        qCritical().noquote() << Q_FUNC_INFO << "Failed to unregister " << wordListPath;

    if (wl.isEmpty()) {
        QMessageBox::warning(
            parent,
            QObject::tr("Word List Missing"),
            QObject::tr("No words were found in:\n%1\n\n"
                        "To use the password generator, you must first select a wordlist "
                        "from Preferences, on the Passwords tab under Word List.")
                .arg(wordListPath)
            );
        return;
    }

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *instruction = new QLabel(
        "Choose the number of words and generate a secure password.\n"
        "You can copy the result to your clipboard.",
        &dlg
        );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    QLabel *wordCountLabel = new QLabel("Words:", &dlg);
    QSpinBox *wordCountSpin = new QSpinBox(&dlg);
    wordCountSpin->setRange(2, 10);
    wordCountSpin->setValue(settings.getGeneratedPasswordLength());

    QPushButton *generateBtn = new QPushButton("&Generate", &dlg);
    QPushButton *copyBtn = new QPushButton("&Copy", &dlg);
    QPushButton *inspectBtn = new QPushButton("&Inspect", &dlg);

    controlLayout->addWidget(wordCountLabel);
    controlLayout->addWidget(wordCountSpin);
    controlLayout->addStretch();
    controlLayout->addWidget(generateBtn);
    controlLayout->addWidget(copyBtn);
    controlLayout->addWidget(inspectBtn);

    layout->addLayout(controlLayout);

    QLineEdit *passwordEdit = new QLineEdit(&dlg);
    passwordEdit->setObjectName("editGenerateNewPassword");
    passwordEdit->setReadOnly(true);
    layout->addWidget(passwordEdit);

    passwordEdit->setText(passwordGenerator::generatePassword(wl, wordCountSpin->value(), 100));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *closeBtn = new QPushButton("&Close", &dlg);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    QObject::connect(generateBtn, &QPushButton::clicked,
                     &dlg,
                     [wl, wordCountSpin, passwordEdit]() {
                         passwordEdit->setText(passwordGenerator::generatePassword(
                             wl, wordCountSpin->value(), 100));
                     });

    QObject::connect(copyBtn, &QPushButton::clicked,
                     &dlg,
                     [passwordEdit, parent] {
                         QApplication::clipboard()->setText(passwordEdit->text());

                         QWidget *p = parent;
                         while (p && !qobject_cast<QMainWindow*>(p))
                             p = p->parentWidget();

                         if (auto *mw = qobject_cast<QMainWindow*>(p))
                             mw->statusBar()->showMessage("Copied to clipboard", 3000);
                     });

    QObject::connect(inspectBtn, &QPushButton::clicked,
                     &dlg,
                     [passwordEdit, &dlg]() {
                         PasswordInspectorDialog inspector(passwordEdit->text(), &dlg);
                         inspector.exec();
                     });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    QObject::connect(wordCountSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     &dlg,
                     [wl, passwordEdit, &settings](int newValue) {
                         settings.setGeneratedPasswordLength(newValue);
                         passwordEdit->setText(passwordGenerator::generatePassword(
                             wl, newValue, 100));
                     });

    dlg.resize(500, 150);
    dlg.exec();
}

}
