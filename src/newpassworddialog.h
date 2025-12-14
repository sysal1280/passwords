#ifndef NEWPASSWORDDIALOG_H
#define NEWPASSWORDDIALOG_H

#include <QDialog>
#include <QList>
#include "keyentry.h"

namespace Ui {
class NewPasswordDialog;
}

class NewPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewPasswordDialog(QWidget *parent = nullptr);
    ~NewPasswordDialog();

    QByteArray toJson() const;
    void setKeys(const QList<KeyEntry> &keys);
    QStringList getAllKeys() const;
    QStringList getCheckedKeys() const;
    QString URL;
    QString Description;
    QString AppName;
    QString PublicAppName;

    void openPassword();
    void openCredentials(QString username, QString password, QString secretOptCode, int length);
    void openNote(QString note);

private slots:
    void onNotesContextMenu(const QPoint &pos);
    void onCredentialsContextMenu(const QPoint &pos);
    void validateForm();

private:
    Ui::NewPasswordDialog *ui;
    QList<KeyEntry> m_keys;
    void suggestFields();
};

#endif // NEWPASSWORDDIALOG_H
