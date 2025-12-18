#pragma once
#include <QString>
#include <QStringList>
#include <QWidget>
#include "settings.h"

namespace PasswordDialog {
void showPasswordGenerator(QWidget *parent,
                           const QString &title,
                           const QStringList &wordList);
}
