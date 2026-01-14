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
