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

namespace PasswordDialog {

void showPasswordGenerator(QWidget *parent,
                           const QString &title,
                           const QStringList & /*wordList*/)
{
    Settings settings;

QString wordListPath = loadWordlistResource(parent, Q_FUNC_INFO);

    // Load wordlist from Settings path
    QStringList wl = passwordGenerator::loadWordList(settings.getWordListFile());

    // Unregister after loading
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

    // Create dialog
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowMaximizeButtonHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *instruction = new QLabel(
        "Choose the number of words and generate a secure passphrase.\n"
        "You can copy the result to your clipboard.",
        &dlg
        );
    instruction->setWordWrap(true);
    instruction->setAlignment(Qt::AlignLeft);
    layout->addWidget(instruction);

    // Controls
    QHBoxLayout *controlLayout = new QHBoxLayout;
    QLabel *wordCountLabel = new QLabel("Words:", &dlg);
    QSpinBox *wordCountSpin = new QSpinBox(&dlg);
    wordCountSpin->setRange(2, 10);
    wordCountSpin->setValue(settings.getGeneratedPasswordLength());

    QPushButton *generateBtn = new QPushButton("&Generate", &dlg);
    QPushButton *copyBtn = new QPushButton("&Copy", &dlg);

    controlLayout->addWidget(wordCountLabel);
    controlLayout->addWidget(wordCountSpin);
    controlLayout->addWidget(generateBtn);
    controlLayout->addWidget(copyBtn);
    layout->addLayout(controlLayout);

    // Password display
    QLineEdit *passwordEdit = new QLineEdit(&dlg);
    passwordEdit->setReadOnly(true);
    layout->addWidget(passwordEdit);

    // Pre‑generate a password using the restored spinbox value
    passwordEdit->setText(passwordGenerator::generatePassword(wl, wordCountSpin->value(), 100));

    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *closeBtn = new QPushButton("&Close", &dlg);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    // Connections
    QObject::connect(generateBtn, &QPushButton::clicked,
                     &dlg,
                     [wl, wordCountSpin, passwordEdit]() {
                         QString pwd = passwordGenerator::generatePassword(wl,
                                                                           wordCountSpin->value(),
                                                                           100);
                         passwordEdit->setText(pwd);
                     });

    QObject::connect(copyBtn, &QPushButton::clicked,
                     &dlg,
                     [passwordEdit] {
                         QApplication::clipboard()->setText(passwordEdit->text());
                     });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    QObject::connect(wordCountSpin,
                     QOverload<int>::of(&QSpinBox::valueChanged),
                     &dlg,
                     [wl, passwordEdit, &settings](int newValue) {
                         settings.setGeneratedPasswordLength(newValue);
                         QString pwd = passwordGenerator::generatePassword(wl, newValue, 100);
                         passwordEdit->setText(pwd);
                     });

    dlg.resize(500, 150);
    dlg.exec();
}

} // namespace PasswordDialog
