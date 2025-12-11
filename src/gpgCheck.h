#ifndef GPGCHECK_H
#define GPGCHECK_H

#include <QWidget>

void checkGpgKeys(QWidget* parent = nullptr);
bool isToolAvailable(const QString &toolName);
bool isStrong(const QString &str);
static QStringList checkKeysWithGpg(const QStringList &keys);

#endif // GPGCHECK_H
