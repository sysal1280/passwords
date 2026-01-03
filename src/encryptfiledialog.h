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


#ifndef ENCRYPTFILEDIALOG_H
#define ENCRYPTFILEDIALOG_H

#include "constants.h"
#include "gpgcheck.h"
#include "passworddialog.h"
#include "randomnoisedialog.h"
#include "settings.h"
#include "utils.h"

#include <mainwindow.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>


class EncryptFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EncryptFileDialog(QWidget *parent = nullptr, QString windowTitle = "")
        : QDialog(parent)
    {
        setWindowTitle(windowTitle);

        auto *mainLayout = new QVBoxLayout(this);

        // --- Form layout for aligned fields ---
        auto *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Input file row
        auto *inputRow = new QWidget(this);
        auto *inputLayout = new QHBoxLayout(inputRow);
        inputLayout->setContentsMargins(0, 0, 0, 0);

        m_inputEdit = new QLineEdit(this);
        m_inputEdit->setReadOnly(true);
        auto *browseBtn = new QPushButton(tr("Browse..."), this);

        inputLayout->addWidget(m_inputEdit);
        inputLayout->addWidget(browseBtn);

        form->addRow(tr("Input file:"), inputRow);

        // Output file row
        m_outputEdit = new QLineEdit(this);
        m_outputEdit->setReadOnly(true);
        form->addRow(tr("Output file:"), m_outputEdit);

        // Password rows
        m_passEdit = new QLineEdit(this);
        m_passConfirmEdit = new QLineEdit(this);

        m_passEdit->setEchoMode(settings.getEchoMode());
        m_passEdit->setPlaceholderText(tr("password"));
        m_passConfirmEdit->setEchoMode(settings.getEchoMode());
        m_passConfirmEdit->setPlaceholderText(tr("confirm password"));

        // Password row with Generate/Noise toolbutton
        auto *passRow = new QWidget(this);
        auto *passLayout = new QHBoxLayout(passRow);
        passLayout->setContentsMargins(0, 0, 0, 0);

        passLayout->addWidget(m_passEdit);
        passLayout->addWidget(m_passConfirmEdit);

        // --- QToolButton for Generate Password / Random Noise ---
        QToolButton *toolBtn = new QToolButton(this);
        toolBtn->setText(tr("Generate"));
        toolBtn->setPopupMode(QToolButton::MenuButtonPopup);

        QMenu *menu = new QMenu(toolBtn);
        QAction *optA = menu->addAction(QIcon(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"), tr("Generate Password"));
        optA->setShortcut(QKeySequence(Qt::Key_F5));
        QAction *optB = menu->addAction(QIcon(":/menus/glyphs/grain_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg"), tr("Random Noise"));
        optB->setShortcut(QKeySequence(Qt::Key_F6));
        toolBtn->setMenu(menu);

        passLayout->addWidget(toolBtn);

        form->addRow(tr("Password:"), passRow);

        // --- Connect default click (Generate Password) ---
        connect(toolBtn, &QToolButton::clicked, this, [this]() {
            PasswordDialog::showPasswordGenerator(
                this,
                tr("Generate Password"),
                {}
                );
        });

        // --- Connect menu option A ---
        connect(optA, &QAction::triggered, this, [this]() {
            PasswordDialog::showPasswordGenerator(
                this,
                tr("Generate Password"),
                {}
                );
        });

        // --- Connect menu option B ---
        connect(optB, &QAction::triggered, this, [this]() {
            RandomNoiseDialog::showRandomNoiseGenerator(this);
        });


        // ASCII armor checkbox
        m_asciiCheck = new QCheckBox(this);
        m_asciiCheck->setChecked(false);
        form->addRow(tr("ASCII armor:"), m_asciiCheck);

        connect(m_asciiCheck, &QCheckBox::toggled,
                this, &EncryptFileDialog::onAsciiToggled);

        mainLayout->addLayout(form);

        // --- Buttons (OK, Cancel, Help) ---
        auto *btnBox = new QDialogButtonBox(
            QDialogButtonBox::Ok |
                QDialogButtonBox::Cancel |
                QDialogButtonBox::Help,
            Qt::Horizontal,
            this
            );
        mainLayout->addWidget(btnBox);

        // Connections
        connect(browseBtn, &QPushButton::clicked,
                this, &EncryptFileDialog::onBrowse);

        connect(btnBox, &QDialogButtonBox::accepted,
                this, &EncryptFileDialog::validateAndAccept);

        connect(btnBox, &QDialogButtonBox::rejected,
                this, &QDialog::reject);

        // --- Help button connection ---
        connect(btnBox->button(QDialogButtonBox::Help),
                &QPushButton::clicked,
                this,
                [this]() {

                    checkHelpReachable([this](bool reachable) {
                        if (reachable) {
                            // Open the online help page for encrypt-file
                            const QUrl url(getHelpBaseUrl("encrypt-file"));
                            QDesktopServices::openUrl(url);
                        } else {
                            // Fallback to helper process
                            MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
                            if (!mw) {
                                QMessageBox::warning(
                                    this,
                                    tr("Help Error"),
                                    tr("Help system unavailable: parent window is not MainWindow.")
                                    );
                                return;
                            }

                            mw->launchHelperProcess(QStringLiteral("encrypt-file"));
                        }
                    });
                });

        // Resize relative to parent
        if (parent) {
            int w = parent->width() * 3 / 4;
            setFixedSize(w, sizeHint().height()+20);
        }
    }

    QString inputFile() const { return m_inputEdit->text(); }
    QString outputFile() const { return m_outputEdit->text(); }
    QString password()  const { return m_passEdit->text(); }
    bool asciiArmor() const { return m_asciiCheck->isChecked(); }

private slots:
    void onBrowse()
    {
        QString file = QFileDialog::getOpenFileName(
            this,
            tr("Select File to Encrypt"),
            QString(),
            tr("All Files (*)")
            );
        if (file.isEmpty())
            return;

        m_inputEdit->setText(file);

        // Auto-suggest output file
        if (m_outputEdit->text().isEmpty() ||
            m_outputEdit->text() == m_lastSuggestedOutput) {

            QString suggested = file + (m_asciiCheck->isChecked() ? ".asc" : ".gpg");
            m_outputEdit->setText(suggested);
            m_lastSuggestedOutput = suggested;
        }
    }

    void validateAndAccept()
    {
        QString in  = m_inputEdit->text().trimmed();
        QString out = m_outputEdit->text().trimmed();
        QString p1  = m_passEdit->text();
        QString p2  = m_passConfirmEdit->text();

        if (in.isEmpty()) {
            QMessageBox::warning(this, tr("Missing Input"),
                                 tr("Please select an input file."));
            return;
        }

        if (!QFileInfo::exists(in)) {
            QMessageBox::warning(this, tr("Invalid Input"),
                                 tr("The selected input file does not exist."));
            return;
        }

        if (out.isEmpty()) {
            QMessageBox::warning(this, tr("Missing Output"),
                                 tr("Please specify an output file."));
            return;
        }

        if (p1.isEmpty()) {
            QMessageBox::warning(this, tr("No Password"),
                                 tr("You must enter a password."));
            return;
        }

        if (p1 != p2) {
            QMessageBox::warning(this, tr("Password Mismatch"),
                                 tr("The passwords do not match. Please re-enter them."));
            m_passEdit->clear();
            m_passConfirmEdit->clear();
            m_passEdit->setFocus();
            return;
        }

        if (!isStrong(p1))
        {
            if (!warnAndContinue())
                return;
        }

        accept();
    }

    void onAsciiToggled(bool checked)
    {
        QString inFile = m_inputEdit->text().trimmed();
        if (inFile.isEmpty())
            return;

        QString currentOut = m_outputEdit->text().trimmed();

        // Strip any existing .gpg or .asc extension
        QString base = currentOut;
        if (base.endsWith(".gpg", Qt::CaseInsensitive))
            base.chop(4);
        else if (base.endsWith(".asc", Qt::CaseInsensitive))
            base.chop(4);
        else if (base == m_lastSuggestedOutput) {
            // fallback to input file if output was auto-suggested
            base = inFile;
        }

        QString suggested = base + (checked ? ".asc" : ".gpg");
        m_outputEdit->setText(suggested);
        m_lastSuggestedOutput = suggested;
    }


private:
    QLineEdit *m_inputEdit = nullptr;
    QLineEdit *m_outputEdit = nullptr;
    QLineEdit *m_passEdit = nullptr;
    QLineEdit *m_passConfirmEdit = nullptr;
    QCheckBox *m_asciiCheck = nullptr;
    QString m_lastSuggestedOutput;
    Settings settings;
};

#endif // ENCRYPTFILEDIALOG_H
