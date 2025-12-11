#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMap>
#include <QAbstractButton>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QMap<QString,QString> keys;
    bool hasKeys() const;


private slots:
    void on_comboBoxLogin_currentIndexChanged(int index);

    void onButtonBoxClicked(QAbstractButton *button);

    void showEvent(QShowEvent *event) ;

    void on_textEdit_textChanged();

private:
    Ui::LoginDialog *ui;
    QByteArray responseHash;
    int errorCount = 0;

};

#endif // LOGINDIALOG_H
