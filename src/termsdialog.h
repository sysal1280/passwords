#ifndef TERMSDIALOG_H
#define TERMSDIALOG_H

#include <QDialog>

class QTextEdit;
class QPushButton;

class TermsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TermsDialog(const QString &termsText, QWidget *parent = nullptr);

private:
    QTextEdit *m_textEdit;
    QPushButton *m_acceptButton;
    QPushButton *m_declineButton;
};

#endif // TERMSDIALOG_H
