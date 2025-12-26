#include "termsdialog.h"
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QCoreApplication>

TermsDialog::TermsDialog(const QString &termsText, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Terms of Use");
    setModal(true);
    resize(600, 400);

    m_textEdit = new QTextEdit(this);
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
}
