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


#include "randomnoisedialog.h"
#include "settings.h"

#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>


namespace RandomNoiseDialog {

static QByteArray generateRandomBytes(int byteCount)
{
    QByteArray buffer(byteCount, Qt::Uninitialized);
    QRandomGenerator::system()->generate(buffer.begin(), buffer.end());
    return buffer;
}

static QString convertBytes(const QByteArray &bytes, int mode)
{
    if (mode == 0)
        return bytes.toBase64();

    if (mode == 1)
        return bytes.toHex();

    // ASCII printable (unbiased)
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "!@#$%^&*()-_=+[]{};:,.<>/?";

    const int charsetSize = sizeof(charset) - 1;
    const int limit = 256 - (256 % charsetSize);

    QString out;
    out.reserve(bytes.size());

    for (unsigned char b : bytes)
    {
        while (b >= limit) {
            b = QRandomGenerator::system()->generate() & 0xFF;
        }
        out.append(charset[b % charsetSize]);
    }

    return out;
}

static double estimateEntropyBits(const QByteArray &data)
{
    if (data.isEmpty())
        return 0.0;

    int counts[256] = {0};
    for (unsigned char c : data)
        counts[c]++;

    int maxCount = 0;
    for (int i = 0; i < 256; ++i)
        if (counts[i] > maxCount)
            maxCount = counts[i];

    const double pMax = double(maxCount) / double(data.size());

    return -std::log2(pMax);
}

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

    QLabel *lenLabel = new QLabel("Length:", &dlg);
    QSpinBox *lenSpin = new QSpinBox(&dlg);
    lenSpin->setRange(4, 4096);
    lenSpin->setSuffix(" bytes");

    {
        Settings settings;
        lenSpin->setValue(settings.getRandomNoiseLength());
    }

    QLabel *formatLabel = new QLabel("Format:", &dlg);
    QComboBox *formatCombo = new QComboBox(&dlg);
    formatCombo->addItem("Base64");
    formatCombo->addItem("Hex");
    formatCombo->addItem("ASCII printable");

    {
        Settings settings;
        formatCombo->setCurrentIndex(settings.getRandomNoiseOption());
    }

    controlLayout->addWidget(lenLabel);
    controlLayout->addWidget(lenSpin);
    controlLayout->addWidget(formatLabel);
    controlLayout->addWidget(formatCombo);
    controlLayout->addStretch();

    layout->addLayout(controlLayout);

    // Output
    QTextEdit *outputEdit = new QTextEdit(&dlg);
    outputEdit->setObjectName("textEditRandomNoise");
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

    copyBtn->setFocus();

    layout->addLayout(buttonLayout);

    auto regenerate = [&]() {
        int len = lenSpin->value();
        int mode = formatCombo->currentIndex();

        QByteArray bytes = generateRandomBytes(len);
        QString out = convertBytes(bytes, mode);

        outputEdit->setPlainText(out);

        double entropy = estimateEntropyBits(bytes);
        entropyLabel->setText(QString("Min‑entropy: %1 bits per byte").arg(entropy));
    };

    regenerate();

    QObject::connect(lenSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dlg,
                     [&](int value){
                         Settings settings;
                         settings.setRandomNoiseLength(value);
                         regenerate();
                     });

    QObject::connect(formatCombo, &QComboBox::currentIndexChanged,
                     &dlg,
                     [&](int index){
                         Settings settings;
                         settings.setRandomNoiseOption(index);
                         regenerate();
                     });

    QObject::connect(copyBtn, &QPushButton::clicked,
                     &dlg, [&]() {
                         QApplication::clipboard()->setText(outputEdit->toPlainText());

                         // Try to find a QMainWindow parent
                         QWidget *p = parent;
                         while (p && !qobject_cast<QMainWindow*>(p))
                             p = p->parentWidget();

                         if (auto *mw = qobject_cast<QMainWindow*>(p)) {
                             mw->statusBar()->showMessage("Copied to clipboard", 3000);
                         }

                         //optional close on cpy.
                         Settings settings;
                         if (settings.closeOnCopy())
                         {
                             dlg.accept();
                         }
                     });


    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.resize(650, 220);
    dlg.exec();
}

}
