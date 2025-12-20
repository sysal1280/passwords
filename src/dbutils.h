#ifndef DBUTILS_H
#define DBUTILS_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>

inline void showDbNotOpenError(const QWidget *parent,
                               const QSqlDatabase &db,
                               const char *caller)
{
    qCritical().noquote()
    << caller
    << "Database not opened."
    << db.lastError().text();

    QMessageBox::critical(
        const_cast<QWidget*>(parent),
        QApplication::applicationName(),
        "No database open."
        );
}

inline void showQueryError(const QWidget *parent,
                           const QSqlQuery &query,
                           const char *caller)
{
    qCritical().noquote()
    << caller
    << "SQL query failed:"
    << query.lastError().text()
    << "\nQuery text:"
    << query.lastQuery();

    QMessageBox::critical(
        const_cast<QWidget*>(parent),
        QApplication::applicationName(),
        "A database query failed. Check logs for details."
        );
}

#endif // DBUTILS_H
