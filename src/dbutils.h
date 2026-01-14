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

#include "dataobfuscator.h"
#include "keyentry.h"

#include <QApplication>
#include <QList>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <quuid.h>


inline void showDbNotOpenError(const QWidget *parent,
                               const QSqlDatabase &db,
                               const char *caller)
{
        QApplication::restoreOverrideCursor();
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
    QApplication::restoreOverrideCursor();
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
        QApplication::restoreOverrideCursor();
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

inline bool isKeyExported(const QWidget *parent, const QSqlDatabase &db)
{
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT value FROM app_info WHERE key = 'app_key'");

    if (!query.exec()) {
        showQueryError(parent, query, Q_FUNC_INFO);
        return false;
    }

    if (!query.first())
        return false;

    const QString value = query.value(0).toString().trimmed().toLower();
    return (value == "exported");
}


namespace DbUtils {

inline QList<KeyEntry> fetchKeys(QWidget *errorParent)
{
    QString connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<KeyEntry> keys;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(qApp->property("dbFile").toString());

        if (db.open()) {

            QSqlQuery query(db);
            query.setForwardOnly(true);

            if (query.exec("SELECT id, label, key FROM keys")) {

                while (query.next()) {
                    KeyEntry entry;
                    entry.id    = query.value(0).toInt();
                    entry.label = DataObfuscator::deobfuscate(
                        query.value(1).toString(),
                        qApp->property("appKey").toByteArray()
                        );
                    entry.key   = DataObfuscator::deobfuscate(
                        query.value(2).toString(),
                        qApp->property("appKey").toByteArray()
                        );
                    keys.append(entry);
                }

            } else {
                showQueryError(errorParent, query, Q_FUNC_INFO);
            }

        } else {
            showDbNotOpenError(errorParent, db, Q_FUNC_INFO);
        }
    }

    QSqlDatabase::removeDatabase(connName);
    return keys;
}

} // namespace DbUtils

#endif // DBUTILS_H
