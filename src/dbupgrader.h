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


#ifndef DATABASEUPGRADER_H
#define DATABASEUPGRADER_H

#include <QString>
#include <QSqlDatabase>

class DatabaseUpgrader
{
public:
    explicit DatabaseUpgrader(QSqlDatabase db);

    // Returns true if DB is now at the correct schema version
    bool upgradeToLatest(const QString &expectedSignature,
                         const QString &expectedSchemaVersion,
                         QString &errorMessage);

private:
    QSqlDatabase m_db;

    QString readSignature();
    QString readSchemaVersion();
    bool writeSchemaVersion(const QString &version, const QString &signature);

    // Upgrade steps
    bool upgrade_1_to_2(QString &error);
    bool upgrade_2_to_3(QString &error);
    bool upgrade_3_to_4(QString &error);

    // Helper to run SQL safely
    bool execQuery(const QString &sql, QString &error);
};

#endif
