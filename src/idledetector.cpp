#include "idledetector.h"
#include <QApplication>
#include <QEvent>

IdleDetector::IdleDetector(int idleSeconds, QObject *parent)
    : QObject(parent),
      m_idleSeconds(idleSeconds)
{
    qApp->installEventFilter(this);

    m_timer.start();

    m_checkTimer.setInterval(1000); // check every second
    connect(&m_checkTimer, &QTimer::timeout, this, &IdleDetector::checkIdle);
    m_checkTimer.start();
}

bool IdleDetector::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
        case QEvent::Type::MouseMove:
        case QEvent::Type::MouseButtonPress:
        case QEvent::Type::KeyPress:
        case QEvent::Type::Wheel:
        case QEvent::Type::TouchBegin:
        case QEvent::Type::TabletMove:
        case QEvent::Type::TabletPress:
            m_timer.restart();
            if (m_isIdle) {
                m_isIdle = false;
                emit activity();
            }
            break;

        default:
            break;
    }

    return QObject::eventFilter(obj, event);
}

void IdleDetector::checkIdle()
{
    if (!m_isIdle && m_timer.elapsed() >= m_idleSeconds * 1000) {
        m_isIdle = true;
        emit idle();
    }
}

