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


#ifndef DBUTILS_H
#define DBUTILS_H

#include <QCoreApplication>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

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
        QCoreApplication::applicationName(),
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
        QCoreApplication::applicationName(),
        QString("A database query failed.\n%1")
            .arg(query.lastError().text())
        );
}

inline void showTransactionError(const QWidget *parent,
                                 const QSqlDatabase &db,
                                 const char *caller)
{
    qCritical().noquote()
    << caller
    << "Failed to start or commit transaction:"
    << db.lastError().text();

    QMessageBox::critical(
        const_cast<QWidget*>(parent),
        QCoreApplication::applicationName(),
        QString("A database transaction failed.\n%1")
            .arg(db.lastError().text())
        );
}

#endif // DBUTILS_H
