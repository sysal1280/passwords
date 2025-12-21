#include "randomnoisedialog.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>
#include <QComboBox>
#include <QRandomGenerator>
#include <QByteArray>

namespace RandomNoiseDialog {

// ------------------------------------------------------------
// Generate random bytes
// ------------------------------------------------------------
static QByteArray generateRandomBytes(int byteCount)
{
    QByteArray buffer(byteCount, Qt::Uninitialized);
    QRandomGenerator::system()->generate(buffer.begin(), buffer.end());
    return buffer;
}

// ------------------------------------------------------------
// Convert bytes to selected format
// ------------------------------------------------------------
static QString convertBytes(const QByteArray &bytes, int mode)
{
    // mode: 0 = Base64, 1 = Hex, 2 = ASCII printable
    if (mode == 0)
        return bytes.toBase64();

    if (mode == 1)
        return bytes.toHex();

    // ASCII printable
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "!@#$%^&*()-_=+[]{};:,.<>/?";

    QString out;
    out.reserve(bytes.size());

    for (unsigned char b : bytes)
        out.append(charset[b % (sizeof(charset) - 1)]);

    return out;
}

// ------------------------------------------------------------
// Entropy estimation
// ------------------------------------------------------------
static double estimateEntropyBits(int byteCount)
{
    return byteCount * 8.0;
}

// ------------------------------------------------------------
// Main dialog
// ------------------------------------------------------------
void showRandomNoiseGenerator(QWidget *parent, const QString &title)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *instruction = new QLabel(
        "Generate cryptographically strong random noise.\n",
        &dlg
        );
    instruction->setWordWrap(true);
    layout->addWidget(instruction);

    // Controls row (NO copy button here anymore)
    QHBoxLayout *controlLayout = new QHBoxLayout;

    QLabel *lenLabel = new QLabel("Length (bytes):", &dlg);
    QSpinBox *lenSpin = new QSpinBox(&dlg);
    lenSpin->setRange(4, 4096);
    lenSpin->setValue(32);

    QLabel *formatLabel = new QLabel("Format:", &dlg);
    QComboBox *formatCombo = new QComboBox(&dlg);
    formatCombo->addItem("Base64");
    formatCombo->addItem("Hex");
    formatCombo->addItem("ASCII printable");

    controlLayout->addWidget(lenLabel);
    controlLayout->addWidget(lenSpin);
    controlLayout->addWidget(formatLabel);
    controlLayout->addWidget(formatCombo);
    controlLayout->addStretch();

    layout->addLayout(controlLayout);

    // Output
    QTextEdit *outputEdit = new QTextEdit(&dlg);
    outputEdit->setReadOnly(true);
    outputEdit->setMinimumHeight(80);
    layout->addWidget(outputEdit);

    // Entropy label
    QLabel *entropyLabel = new QLabel("Entropy: 0 bits", &dlg);
    layout->addWidget(entropyLabel);

    // Bottom buttons: Copy + Close
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    QPushButton *copyBtn = new QPushButton("&Copy", &dlg);
    QPushButton *closeBtn = new QPushButton("&Close", &dlg);

    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(closeBtn);

    layout->addLayout(buttonLayout);

    // ------------------------------------------------------------
    // Live update function
    // ------------------------------------------------------------
    auto regenerate = [&]() {
        int len = lenSpin->value();
        int mode = formatCombo->currentIndex();

        QByteArray bytes = generateRandomBytes(len);
        QString out = convertBytes(bytes, mode);

        outputEdit->setPlainText(out);

        double entropy = estimateEntropyBits(len);
        entropyLabel->setText(QString("Entropy: %1 bits").arg(entropy));
    };

    regenerate();

    QObject::connect(lenSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dlg, [&](int){ regenerate(); });

    QObject::connect(formatCombo, &QComboBox::currentIndexChanged,
                     &dlg, [&](int){ regenerate(); });

    QObject::connect(copyBtn, &QPushButton::clicked,
                     &dlg, [&]() {
                         QApplication::clipboard()->setText(outputEdit->toPlainText());
                     });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.resize(650, 220);
    dlg.exec();
}

} // namespace RandomNoiseDialog
