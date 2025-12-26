#include "dbmaintenance.h"

DbMaintenance::DbMaintenance(QSqlDatabase db, QWidget* parent)
    : QWidget(parent), m_db(db)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_list = new QListWidget(this);
    layout->addWidget(m_list);

    QPushButton* startBtn = new QPushButton("Start Maintenance", this);
    layout->addWidget(startBtn);

    connect(startBtn, &QPushButton::clicked,
            this, &DbMaintenance::startMaintenance);
}

void DbMaintenance::startMaintenance()
{
    m_list->clear();
    m_list->addItem("Starting database maintenance...");

    QThread* thread = QThread::create([this]() {
        runMaintenance();
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void DbMaintenance::addMessage(const QString& msg)
{
    m_list->addItem(msg);
}

void DbMaintenance::runMaintenance()
{
    auto post = [&](const QString& msg){
        QMetaObject::invokeMethod(this, "addMessage",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, msg));
    };

    post("Checking integrity...");
    runQuery("PRAGMA integrity_check;", post);

    post("Running quick_check...");
    runQuery("PRAGMA quick_check;", post);

    post("Analyzing database...");
    runQuery("ANALYZE;", post);

    post("Reindexing...");
    runQuery("REINDEX;", post);

    post("Checking WAL mode...");
    QString journalMode = getSingleValue("PRAGMA journal_mode;", post);

    if (journalMode.toLower() == "wal") {
        post("Running WAL checkpoint...");
        runQuery("PRAGMA wal_checkpoint(FULL);", post);
    } else {
        post("Database not in WAL mode, skipping checkpoint.");
    }

    post("Vacuuming...");
    runQuery("VACUUM;", post);

    post("Maintenance complete.");
}

void DbMaintenance::runQuery(const QString& sql,
                             std::function<void(const QString&)> post)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        post("ERROR: " + q.lastError().text());
    } else {
        post("OK: " + sql);
    }
}

QString DbMaintenance::getSingleValue(const QString& sql,
                                      std::function<void(const QString&)> post)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        post("ERROR: " + q.lastError().text());
        return "";
    }
    if (q.next())
        return q.value(0).toString();
    return "";
}
