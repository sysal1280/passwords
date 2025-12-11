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
    QStringList getCheckedKeys() const; // optional if you want only checked
    QString URL;
    QString Description;
    QString AppName;
    QString PublicAppName;

    void openPassword();

    void openCredentials(QString username, QString password, QString secretOptCode, int length);

    void openNote(QString note);

private slots:
    void on_buttonBox_accepted();
    void onNotesContextMenu(const QPoint &pos);
    void onCredentialsContextMenu(const QPoint &pos);
    void validateForm();

    void on_lineEditPublicAppName_editingFinished();

    void on_pushButtonGenerate_clicked();

private:
    Ui::NewPasswordDialog *ui;
    QList<KeyEntry> m_keys;
};

#endif // NEWPASSWORDDIALOG_H
