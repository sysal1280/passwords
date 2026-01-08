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


#include "testloader.h"
#include "dataobfuscator.h"

#include <QApplication>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QtConcurrent>

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// ---------------------------------------------------------
// TestLoader core
// ---------------------------------------------------------

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

    return -1;
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

    QVector<int> existingIds;

    for (int i = 0; i < count; ++i) {
        int parentId = QVariant().toInt();

        if (!existingIds.isEmpty() && QRandomGenerator::global()->bounded(100) < 70) {
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
    qDebug() << "\n=== generateApplications() START ===";
    qDebug() << "Requested application count:" << count;

    QSqlDatabase db;
    QString connName;

    if (!openDb(db, connName)) {
        qWarning() << "Failed to open DB";
        return false;
    }

    db.transaction();

    QSqlQuery insert(db);
    insert.prepare(
        "INSERT INTO application (category_id, application_name, data, created) "
        "VALUES (?, ?, ?, ?)");

    for (int i = 0; i < count; ++i) {

        qDebug() << "\n--- Application" << i+1 << "of" << count << "---";

        // 1. Pick category
        qDebug() << "Step: Choosing category";
        int catId = randomExistingCategoryId(db);
        qDebug() << "Chosen category ID:" << catId;

        if (catId < 0) {
            qWarning() << "No categories exist; cannot generate applications.";
            db.rollback();
            closeDb(connName);
            return false;
        }

        // 2. Generate app name
        QString appName = QString("PWD_%1").arg(randomString(QRandomGenerator::global()->bounded(4, 21)));
        QString obAppName = DataObfuscator::obfuscate(appName, appKey);
        qDebug() << "Generated app name:" << appName;

        // 3. Generate JSON
        qDebug() << "Step: Generating JSON";
        QString json = fakeJson();
        qDebug() << "JSON payload:" << json;

        // 4. Get GPG key
        qDebug() << "Step: Getting GPG key";
        QString dbPath = qApp->property("dbFile").toString();
        QByteArray keyBytes = qApp->property("appKey").toByteArray();

        qDebug() << "DB file used for GPG key:" << dbPath;
        qDebug() << "App key length:" << keyBytes.size();

        QStringList recipients;
        QString gpgKey = getRandomGpgKey(dbPath, keyBytes);
        qDebug() << "Random GPG key returned:" << gpgKey;

        recipients << gpgKey;

        if (gpgKey.isEmpty()) {
            qWarning() << "ERROR: getRandomGpgKey() returned EMPTY key!";
            db.rollback();
            closeDb(connName);
            return false;
        }

        // 5. Encrypt JSON
        qDebug() << "Step: Encrypting JSON with GPG";
        QByteArray encrypted = encryptWithGpg(recipients, json);
        qDebug() << "Encrypted size:" << encrypted.size();

        if (encrypted.isEmpty()) {
            qWarning() << "ERROR: GPG encryption failed — encrypted data is EMPTY";
            db.rollback();
            closeDb(connName);
            return false;
        }

        // 6. Obfuscate encrypted payload
        qDebug() << "Step: Obfuscating encrypted payload";
        QString obPayload = DataObfuscator::obfuscate(QString::fromUtf8(encrypted), appKey);

        // 7. Insert into DB
        qDebug() << "Step: Inserting into DB";
        insert.bindValue(0, catId);
        insert.bindValue(1, obAppName);
        insert.bindValue(2, obPayload);
        insert.bindValue(3, QDateTime::currentSecsSinceEpoch());

        if (!insert.exec()) {
            qWarning() << "Application insert failed:" << insert.lastError();
            db.rollback();
            closeDb(connName);
            return false;
        }

        qDebug() << "Application inserted successfully.";
        emit progress((i * 100) / count);

    }

    db.commit();
    closeDb(connName);

    qDebug() << "=== generateApplications() COMPLETE ===\n";
    return true;
}

// ---------------------------------------------------------
// Dialog implementation
// ---------------------------------------------------------

TestLoaderDialog::TestLoaderDialog(TestLoader *loader, QWidget *parent)
    : QDialog(parent),
    loader(loader)
{
    setWindowTitle("Test Data Generator");
    setMinimumWidth(350);

    auto *layout = new QVBoxLayout(this);

    auto *catLayout = new QHBoxLayout();
    catLayout->addWidget(new QLabel("Categories:"));
    categoryCountSpin = new QSpinBox();
    categoryCountSpin->setRange(0, 1000000);
    categoryCountSpin->setValue(50);
    catLayout->addWidget(categoryCountSpin);
    layout->addLayout(catLayout);

    auto *depthLayout = new QHBoxLayout();
    depthLayout->addWidget(new QLabel("Max Depth:"));
    categoryDepthSpin = new QSpinBox();
    categoryDepthSpin->setRange(1, 10);
    categoryDepthSpin->setValue(3);
    depthLayout->addWidget(categoryDepthSpin);
    layout->addLayout(depthLayout);

    auto *appLayout = new QHBoxLayout();
    appLayout->addWidget(new QLabel("Applications:"));
    appCountSpin = new QSpinBox();
    appCountSpin->setRange(0, 1000000);
    appCountSpin->setValue(200);
    appLayout->addWidget(appCountSpin);
    layout->addLayout(appLayout);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedHeight(18);
    layout->addWidget(progressBar);

    connect(loader, &TestLoader::progress, this, [=](int value) {
        progressBar->setValue(value);
    });


    // --- Buttons ---
    auto *buttonLayout = new QHBoxLayout();

    // Add stretch to push buttons to the right
    buttonLayout->addStretch(1);

    runButton = new QPushButton("Run");
    closeButton = new QPushButton("Close");

    // Make buttons smaller (fixed size instead of expanding)
    runButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    connect(runButton, &QPushButton::clicked, this, &TestLoaderDialog::onRunClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
}

void TestLoaderDialog::onRunClicked()
{
    runButton->setEnabled(false);
    progressBar->setValue(0);

    int catCount = categoryCountSpin->value();
    int depth    = categoryDepthSpin->value();
    int appCount = appCountSpin->value();

    QFuture<void> future = QtConcurrent::run([=]() {
        if (catCount > 0)
            loader->generateCategories(catCount, depth);

        if (appCount > 0)
            loader->generateApplications(appCount);
    });

    // Monitor progress
    auto *watcher = new QFutureWatcher<void>(this);

    connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
        progressBar->setValue(100);
        QMessageBox::information(this, "Done", "Test data generation complete.");
        runButton->setEnabled(true);
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

// ---------------------------------------------------------
// Public method to show dialog
// ---------------------------------------------------------

void TestLoader::showDialog(QWidget *parent)
{
    TestLoaderDialog dlg(this, parent);
    dlg.exec();
}

QString TestLoader::fakeJson()
{
    QJsonObject root;

    // -----------------------------
    // Generate credentials (1–10, skewed low)
    // -----------------------------
    QJsonArray credentials;

    // Skew toward lower numbers by squaring a random value
    double r = QRandomGenerator::global()->bounded(1000) / 1000.0; // 0.0–1.0
    double skewed = 1.0 + std::pow(r * 2.0, 2.0); // maps into ~1–5 range, skewed low
    int credCount = qBound(1, int(skewed), 10);


    for (int i = 0; i < credCount; ++i) {
        QJsonObject cred;

        cred["length"] = 6;
        cred["username"] = randomString(QRandomGenerator::global()->bounded(3, 12));
        cred["password"] = randomString(QRandomGenerator::global()->bounded(10, 24));

        // 20% chance of having an OTP code
        if (QRandomGenerator::global()->bounded(100) < 20) {
            cred["secretOtpCode"] = randomString(6);
        } else {
            cred["secretOtpCode"] = "";
        }

        // 30% chance of having an extra field
        if (QRandomGenerator::global()->bounded(100) < 30) {
            cred["extra_info"] = QString("Extra_%1").arg(randomString(5));
        }

        credentials.append(cred);
    }

    root["credentials"] = credentials;

    // -----------------------------
    // Generate notes (1–6)
    // -----------------------------
    QJsonArray notes;
    int noteCount = QRandomGenerator::global()->bounded(1, 6);

    for (int i = 0; i < noteCount; ++i) {
        QJsonObject noteObj;

        // Random note length between 20–80 chars
        noteObj["content"] = QString("Note %1: %2")
                                 .arg(i + 1)
                                 .arg(randomTextWithSpaces(16, 512));

        // 25% chance of a "tag"
        if (QRandomGenerator::global()->bounded(100) < 25) {
            noteObj["tag"] = QString("Tag_%1").arg(randomString(4));
        }

        notes.append(noteObj);
    }

    root["notes"] = notes;

    // -----------------------------
    // Other fields
    // -----------------------------
    root["description"] = QString("Description_%1").arg(randomString(6));
    root["private_name"] = QString("Private_%1").arg(randomString(5));
    root["url"] = QString("https://%1.com").arg(randomString(6));

    // 15% chance of adding a "metadata" object
    if (QRandomGenerator::global()->bounded(100) < 15) {
        QJsonObject meta;
        meta["created_by"] = randomString(6);
        meta["version"] = QRandomGenerator::global()->bounded(1, 10);
        root["metadata"] = meta;
    }

    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}


QByteArray TestLoader::encryptWithGpg(const QStringList &recipients, const QString &json)
{
    qDebug() << "\n=== encryptWithGpg() START ===";
    qDebug() << "Recipients:" << recipients;
    qDebug() << "Plaintext length:" << json.toUtf8().size();

    QString baseDir = "/dev/shm";
    if (!QFileInfo::exists(baseDir) || !QFileInfo(baseDir).isWritable()) {
        baseDir = QDir::tempPath();
    }
    qDebug() << "Base dir for temp file:" << baseDir;

    QString tempFile = baseDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".asc";
    qDebug() << "Temp output file:" << tempFile;

    QStringList args;
    for (const QString &key : recipients)
        args << "--recipient" << key;

    // Important: read from stdin with "-"
    args << "--batch"
         << "--yes"
         << "--trust-model" << "always"
         << "--encrypt"
         << "--armor"
         << "--output" << tempFile
         << "-";

    qDebug() << "GPG args:" << args;

    QProcess process;
    process.start("gpg", args);

    if (!process.waitForStarted()) {
        qWarning() << "GPG failed to start";
        qDebug() << "GPG error:" << process.errorString();
        return QByteArray();
    }

    process.write(json.toUtf8());
    process.closeWriteChannel();

    bool finished = process.waitForFinished();
    qDebug() << "GPG finished:" << finished
             << "exitCode:" << process.exitCode()
             << "exitStatus:" << process.exitStatus();
    qDebug() << "GPG stdout:" << process.readAllStandardOutput();
    qDebug() << "GPG stderr:" << process.readAllStandardError();

    QFileInfo fi(tempFile);
    qDebug() << "Temp file exists after GPG:" << fi.exists()
             << "size:" << (fi.exists() ? fi.size() : -1);

    QFile f(tempFile);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to read encrypted file";
        qDebug() << "QFile error:" << f.errorString();
        return QByteArray();
    }

    QByteArray encrypted = f.readAll();
    f.close();
    f.remove();

    qDebug() << "Encrypted data length:" << encrypted.size();
    qDebug() << "=== encryptWithGpg() END ===\n";

    return encrypted;
}


QString TestLoader::getRandomGpgKey(const QString &dbFile, const QByteArray &appKey)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "randkey");
    db.setDatabaseName(dbFile);

    if (!db.open()) {
        // Handle DB open error however your app normally does
        qWarning() << "Database failed to open";
        QSqlDatabase::removeDatabase("randkey");
        return QString();
    }

    QSqlQuery query(db);

    // SQLite RANDOM() gives a random row
    if (!query.exec("SELECT key FROM keys ORDER BY RANDOM() LIMIT 1")) {
        qWarning() << "Query failed:" << query.lastError();
        QSqlDatabase::removeDatabase("randkey");
        return QString();
    }

    QString result;

    if (query.next()) {
        QString obfuscatedKey = query.value(0).toString();
        result = DataObfuscator::deobfuscate(obfuscatedKey, appKey);
    }

    QSqlDatabase::removeDatabase("randkey");
    return result;
}

QString TestLoader::randomTextWithSpaces(int minLen, int maxLen)
{
    int length = QRandomGenerator::global()->bounded(minLen, maxLen);
    QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789     ";
    // Note: spaces included at the end

    QString out;
    out.reserve(length);

    for (int i = 0; i < length; ++i) {
        out.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));
    }

    // Clean up: avoid leading/trailing spaces
    return out.trimmed();
}

void TestLoaderDialog::closeEvent(QCloseEvent *event)
{
    if (!runButton->isEnabled()) {
        QMessageBox::warning(this,
                             "Test Data Generator",
                             "Generation in progress. Please wait...");
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

void TestLoaderDialog::reject()
{
    if (!runButton->isEnabled()) {
        QMessageBox::warning(this,
                             "Test Data Generator",
                             "Generation in progress. Please wait...");
        return;
    }

    QDialog::reject();
}
