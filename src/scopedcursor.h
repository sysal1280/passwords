#pragma once
#include <QApplication>
#include <QCursor>

class ScopedCursor {
public:
    explicit ScopedCursor(const QCursor &cursor) {
        QApplication::setOverrideCursor(cursor);
    }
    ~ScopedCursor() {
        QApplication::restoreOverrideCursor();
    }
};
