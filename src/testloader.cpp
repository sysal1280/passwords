#include "testloader.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRandomGenerator>
#include <QUuid>
#include <QDebug>
#include "dataobfuscator.h"
#include <QDateTime>

TestLoader::TestLoader(const QString &dbFile,
                       const QByteArray &appKey,
                       QObject *parent)
    : QObject(parent),
    dbFile(dbFile),
    appKey(appKey)
{
}

bool TestLoader::openDb(QSqlDatabase &db, QString &connName)
{
    connName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbFile);

    if (!db.open()) {
        qWarning() << "TestLoader: DB open failed:" << db.lastError();
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON");

    return true;
}

void TestLoader::closeDb(const QString &connName)
{
    QSqlDatabase::removeDatabase(connName);
}

QString TestLoader::randomString(int length)
{
    const QString chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    QString out;
    out.reserve(length);

    for (int i = 0; i < length; ++i)
        out.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));

    return out;
}

int TestLoader::randomExistingCategoryId(QSqlDatabase &db)
{
    QSqlQuery q("SELECT id FROM categories ORDER BY RANDOM() LIMIT 1", db);
    if (q.next())
        return q.value(0).toInt();

    return -1; // no categories exist
}

bool TestLoader::generateCategories(int count, int maxDepth)
{
    QSqlDatabase db;
    QString connName;

    if (!openDb(db, connName))
        return false;

    db.transaction();

    QSqlQuery insert(db);
    insert.prepare("INSERT INTO categories (parent_id, text) VALUES (?, ?)");

    QVector<int> existingIds; // track IDs for parent selection

    for (int i = 0; i < count; ++i) {
        int parentId = QVariant().toInt(); // NULL by default

        if (!existingIds.isEmpty() && QRandomGenerator::global()->bounded(100) < 70) {
            // 70% chance to assign a parent
            parentId = existingIds.at(QRandomGenerator::global()->bounded(existingIds.size()));
        }

        QString text = QString("Category_%1").arg(randomString(8));
        QString obText = DataObfuscator::obfuscate(text, appKey);

        insert.bindValue(0, parentId == 0 ? QVariant() : QVariant(parentId));
        insert.bindValue(1, obText);

        if (!insert.exec()) {
            qWarning() << "TestLoader: Category insert failed:" << insert.lastError();
            db.rollback();
            closeDb(connName);
            return false;
        }

        existingIds.append(insert.lastInsertId().toInt());
    }

    db.commit();
    closeDb(connName);
    return true;
}

bool TestLoader::generateApplications(int count)
{
    QSqlDatabase db;
    QString connName;

    if (!openDb(db, connName))
        return false;

    db.transaction();

    QSqlQuery insert(db);
    insert.prepare(
        "INSERT INTO application (category_id, application_name, data, created) "
        "VALUES (?, ?, ?, ?)");

    for (int i = 0; i < count; ++i) {
        int catId = randomExistingCategoryId(db);
        if (catId < 0) {
            qWarning() << "TestLoader: No categories exist; cannot generate applications.";
            db.rollback();
            closeDb(connName);
            return false;
        }

        QString appName = QString("App_%1").arg(randomString(10));
        QString payload = QString("Payload_%1").arg(randomString(32));

        // 🔥 Obfuscate BOTH fields
        QString obAppName = DataObfuscator::obfuscate(appName, appKey);
        QString obPayload = DataObfuscator::obfuscate(payload, appKey);

        insert.bindValue(0, catId);
        insert.bindValue(1, obAppName);
        insert.bindValue(2, obPayload);
        insert.bindValue(3, QDateTime::currentSecsSinceEpoch());

        if (!insert.exec()) {
            qWarning() << "TestLoader: Application insert failed:" << insert.lastError();
            db.rollback();
            closeDb(connName);
            return false;
        }
    }

    db.commit();
    closeDb(connName);
    return true;
}
