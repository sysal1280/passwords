#ifndef GPGCHECK_H
#define GPGCHECK_H

#include <QWidget>

void checkGpgKeys(QWidget* parent = nullptr);
bool isToolAvailable(const QString &toolName);
bool isStrong(const QString &str);
bool hasUltimateTrust(const QString &keyId);
bool warnAndContinue();
bool warmupGpg(const QString &recipientKey, QWidget *parent);
static QStringList checkKeysWithGpg(const QStringList &keys);

#endif // GPGCHECK_H
