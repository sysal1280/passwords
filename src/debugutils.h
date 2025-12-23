#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <QObject>

bool isDebuggerAttached();
void startDebuggerMonitor(QObject *parent = nullptr, int intervalMs = 2000);

#endif // DEBUGUTILS_H
