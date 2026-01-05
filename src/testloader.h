#ifndef TESTLOADER_H
#define TESTLOADER_H

#include <QObject>
#include <QSqlDatabase>
#include <QByteArray>
#include <QString>

class TestLoader : public QObject
{
    Q_OBJECT
public:
    explicit TestLoader(const QString &dbFile,
                        const QByteArray &appKey,
                        QObject *parent = nullptr);

    bool generateCategories(int count, int maxDepth = 3);
    bool generateApplications(int count);

private:
    QString dbFile;
    QByteArray appKey;

    bool openDb(QSqlDatabase &db, QString &connName);
    void closeDb(const QString &connName);

    int randomExistingCategoryId(QSqlDatabase &db);
    QString randomString(int length);
};

#endif // TESTLOADER_H
