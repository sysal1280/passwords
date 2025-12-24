#include "passwordDialog.h"
#include "passwordGenerator.h"
#include "settings.h"
#include "utils.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QResource>
#include <QDebug>
#include <QMainWindow>
#include <QStatusBar>
#include <QFrame>
#include <QScrollArea>
#include <QtMath>
#include <QScreen>

namespace PasswordDialog {

// ------------------------------------------------------------
// Password Inspector Dialog
// ------------------------------------------------------------

class PasswordInspectorDialog : public QDialog
{
public:
    PasswordInspectorDialog(const QString &password, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Password Inspector");
        setModal(true);
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

        // Material-style tile theming
        setStyleSheet(
            "QFrame[tileType='upper'] { background-color: #1976D2; color: white; border-radius: 8px; padding: 4px; }"
            "QFrame[tileType='upper']:hover { background-color: #1E88E5; }"

            "QFrame[tileType='lower'] { background-color: #388E3C; color: white; border-radius: 8px; padding: 4px; }"
            "QFrame[tileType='lower']:hover { background-color: #43A047; }"

            "QFrame[tileType='digit'] { background-color: #7B1FA2; color: white; border-radius: 8px; padding: 4px; }"
            "QFrame[tileType='digit']:hover { background-color: #8E24AA; }"

            "QFrame[tileType='symbol'] { background-color: #F57C00; color: white; border-radius: 8px; padding: 4px; }"
            "QFrame[tileType='symbol']:hover { background-color: #FB8C00; }"
            );

        QVBoxLayout *main = new QVBoxLayout(this);

        // Big password label
        QLabel *pwLabel = new QLabel(password, this);
        QFont big; big.setPointSize(20); big.setBold(true);
        pwLabel->setFont(big);
        pwLabel->setAlignment(Qt::AlignCenter);
        pwLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        main->addWidget(pwLabel);

        // Scroll area for tiles
        QScrollArea *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        QWidget *tilesContainer = new QWidget(scroll);
        QHBoxLayout *tiles = new QHBoxLayout(tilesContainer);
        tiles->setSpacing(8);

        for (QChar c : password)
            tiles->addWidget(createCharacterTile(c));

        tiles->addStretch();
        scroll->setWidget(tilesContainer);
        main->addWidget(scroll);

        // Legend (Uppercase / Lowercase / Digit / Symbol) aligned right
        QWidget *legend = new QWidget(this);
        QHBoxLayout *legendLayout = new QHBoxLayout(legend);
        legendLayout->setSpacing(15);
        legendLayout->setContentsMargins(0, 0, 0, 0);
        legendLayout->setAlignment(Qt::AlignRight);

        legendLayout->addWidget(makeLegendItem("#1976D2", "Uppercase"));
        legendLayout->addWidget(makeLegendItem("#388E3C", "Lowercase"));
        legendLayout->addWidget(makeLegendItem("#7B1FA2", "Digit"));
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

        // Determine screen size
        QScreen *screen = QGuiApplication::primaryScreen();
        int screenWidth = screen->geometry().width();

        // Choose percentage based on screen size
        double percent = (screenWidth >= 1920) ? 0.45 : 0.55;

        // Compute width
        int dialogWidth = int(screenWidth * percent);

        // Apply size
        resize(dialogWidth, 300);

    }

private:
    // ------------------------------------------------------------
    // Legend helper
    // ------------------------------------------------------------
    QWidget *makeLegendItem(const QString &color, const QString &label)
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

    // ------------------------------------------------------------
    // Tile creation
    // ------------------------------------------------------------
    QWidget* createCharacterTile(QChar c)
    {
        QFrame *frame = new QFrame;
        frame->setFrameShape(QFrame::NoFrame);
        frame->setProperty("tileType", characterType(c));

        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(8, 6, 8, 6);

        // Top: big character
        QLabel *charLabel = new QLabel(QString(c), frame);
        QFont big; big.setPointSize(18); big.setBold(true);
        charLabel->setFont(big);
        charLabel->setAlignment(Qt::AlignCenter);

        // Bottom: description / name
        QLabel *descLabel = new QLabel(characterDescription(c), frame);
        QFont small; small.setPointSize(8);
        descLabel->setFont(small);
        descLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(charLabel);
        layout->addWidget(descLabel);

        return frame;
    }

    QString characterType(QChar c) const
    {
        if (c.isUpper()) return "upper";
        if (c.isLower()) return "lower";
        if (c.isDigit()) return "digit";
        return "symbol";
    }

    QString symbolName(QChar c) const
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
        default:
            break;
        }

        // Fallback for anything we didn't name explicitly
        return QString("Symbol (U+%1)").arg(QString::number(c.unicode(), 16).toUpper());
    }

    QString characterDescription(QChar c) const
    {
        if (c.isUpper())
            return QString("Uppercase %1").arg(c);

        if (c.isLower())
            return QString("Lowercase %1").arg(c);

        if (c.isDigit())
            return QString("Digit %1").arg(c);

        // Symbol: use our manual name map
        return symbolName(c);
    }

    // ------------------------------------------------------------
    // Password analysis
    // ------------------------------------------------------------
    struct PasswordAnalysis {
        int length = 0;
        int upperCount = 0;
        int lowerCount = 0;
        int digitCount = 0;
        int symbolCount = 0;
        double entropyBits = 0.0;
        QString entropyLabel;
        QString segmentation;
    };

    PasswordAnalysis analyzePassword(const QString &password) const
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

    QString segmentPassword(const QString &password) const
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

    QString formatAnalysisSummary(const PasswordAnalysis &a) const
    {
        QString summary;

        summary += QString("Entropy: %1 bits (%2)\n")
                       .arg(QString::number(a.entropyBits, 'f', 1))
                       .arg(a.entropyLabel);

        summary += QString("Length: %1 characters\n").arg(a.length);

        summary += QString("Character classes: %1 uppercase, %2 lowercase, %3 digits, %4 symbols\n")
                       .arg(a.upperCount)
                       .arg(a.lowerCount)
                       .arg(a.digitCount)
                       .arg(a.symbolCount);

        summary += QString("Segmentation: %1").arg(a.segmentation);

        return summary;
    }
};

// ------------------------------------------------------------
// Password Generator Dialog
// ------------------------------------------------------------

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

} // namespace PasswordDialog
