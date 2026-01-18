#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

class IdleDetector : public QObject {
    Q_OBJECT

public:
    explicit IdleDetector(int idleSeconds, QObject *parent = nullptr);

signals:
    void idle();
    void activity();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void checkIdle();

    QElapsedTimer m_timer;
    QTimer m_checkTimer;
    int m_idleSeconds;
    bool m_isIdle = false;
};

