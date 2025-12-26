/*
 * passwords - A GnuPG based password manager
 *
 * Copyright (C) 2025  Adam.Lanzafame <sysal@tuta.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "debugutils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <fstream>
#elif defined(Q_OS_MAC)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

bool isDebuggerAttached()
{
#if defined(Q_OS_WIN)
    return IsDebuggerPresent() != 0;

#elif defined(Q_OS_LINUX)
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("TracerPid:", 0) == 0) {
            int tracerPid = std::stoi(line.substr(10));
            return tracerPid != 0;
        }
    }
    return false;

#elif defined(Q_OS_MAC)
    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    info.kp_proc.p_flag = 0;

    if (sysctl(mib, 4, &info, &size, nullptr, 0) == 0) {
        return (info.kp_proc.p_flag & P_TRACED) != 0;
    }
    return false;

#else
    return false;
#endif
}

void startDebuggerMonitor(QObject *parent, int intervalMs)
{
    QTimer *timer = new QTimer(parent);

    QObject::connect(timer, &QTimer::timeout, []() {
        qDebug() << Q_FUNC_INFO;
        if (isDebuggerAttached()) {
            qWarning("Debugger detected during runtime — exiting.");
            QCoreApplication::exit(0);
        }
    });

    timer->start(intervalMs);
}

