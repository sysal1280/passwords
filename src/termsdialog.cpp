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


#include "termsdialog.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

TermsDialog::TermsDialog(const QString &termsText, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Terms of Use");
    setModal(true);
    resize(600, 400);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setObjectName("textEditTerms");
    m_textEdit->setReadOnly(true);
    m_textEdit->setPlainText(termsText);

    m_acceptButton = new QPushButton("Accept", this);
    m_declineButton = new QPushButton("Decline", this);

    connect(m_acceptButton, &QPushButton::clicked, this, &TermsDialog::accept);
    connect(m_declineButton, &QPushButton::clicked, this, &TermsDialog::reject);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_acceptButton);
    buttonLayout->addWidget(m_declineButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_textEdit);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    QApplication::restoreOverrideCursor();
}
