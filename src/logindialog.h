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
    void showEvent(QShowEvent *event) ;

private:
    Ui::LoginDialog *ui;
    QByteArray responseHash;
    int errorCount = 0;
    void generateResponse();
    void tryResponse();
    void checkHelperFiles();

};

#endif // LOGINDIALOG_H
