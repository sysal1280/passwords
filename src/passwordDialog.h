#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QString>
#include <QStringList>
#include <QWidget>

namespace PasswordDialog {
void showPasswordGenerator(QWidget *parent,
                           const QString &title,
                           const QStringList &wordList);
}

#endif // PASSWORDDIALOG_H
