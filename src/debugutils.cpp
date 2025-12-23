#include "debugutils.h"
#include <QtGlobal>
#include <QTimer>
#include <QCoreApplication>

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
        if (isDebuggerAttached()) {
            qWarning("Debugger detected during runtime — exiting.");
            QCoreApplication::exit(0);
        }
    });

    timer->start(intervalMs);
}

