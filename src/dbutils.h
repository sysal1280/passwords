#ifndef DBUTILS_H
#define DBUTILS_H

#include <QSqlDatabase>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>

inline void showDbNotOpenError(const QWidget *parent, const QSqlDatabase &db)
{
    qCritical().noquote() << "Database not opened." << db.lastError().text();
    QMessageBox::critical(
        const_cast<QWidget*>(parent),
        QApplication::applicationName(),
        "No database open."
        );
}

#endif // DBUTILS_H
