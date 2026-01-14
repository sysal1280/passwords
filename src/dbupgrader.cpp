#include "dbupgrader.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QRandomGenerator>
#include <QInputDialog>
#include <QRegularExpression>

DatabaseUpgrader::DatabaseUpgrader(QSqlDatabase db)
    : m_db(db)
{
}

QString DatabaseUpgrader::readSignature()
{
    QSqlQuery q(m_db);
    q.exec("SELECT value FROM app_info WHERE key='app_signature'");
    return q.next() ? q.value(0).toString() : QString();
}

QString DatabaseUpgrader::readSchemaVersion()
{
    QSqlQuery q(m_db);
    q.exec("SELECT value FROM app_info WHERE key='schema_version'");
    return q.next() ? q.value(0).toString() : QString();
}

bool DatabaseUpgrader::writeSchemaVersion(const QString &version, const QString &signature)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE app_info SET value=? WHERE key='schema_version'");
    q.addBindValue(version);
    q.exec();

    q.prepare("UPDATE app_info SET value=? WHERE key='app_signature'");
    q.addBindValue(signature);
    return q.exec();
}

bool DatabaseUpgrader::execQuery(const QString &sql, QString &error)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        error = q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseUpgrader::upgradeToLatest(const QString &expectedSignature,
                                       const QString &expectedSchemaVersion,
                                       QString &errorMessage)
{
    QString version = readSchemaVersion();
    QString signature = readSignature();

    qInfo().noquote() << QString("Database schema is v%1 (%2).")
                             .arg(version, signature);

    if (version == expectedSchemaVersion && signature == expectedSignature) {
        qInfo().noquote() << "No database upgrade necessary.";
        return true;
    }

    // Upgrade 1 → 2 (Asteraceae to Myrtaceae)
    if (version == "1") {
        if (!upgrade_1_to_2(errorMessage))
            return false;

        if (!writeSchemaVersion("2", "Myrtaceae")) {
            errorMessage = "Failed to update schema_version to 2.";
            return false;
        }

        version = "2";
        qInfo().noquote() << "Upgraded to schema v2 (Myrtaceae).";
    }

    // Future upgrades:
    // if (version == "2") { ... }
    // if (version == "3") { ... }

    // Final sanity check
    if (version != expectedSchemaVersion) {
        errorMessage = "Database schema is newer than this application supports.";
        return false;
    }

    return true;
}

// ------------------ Upgrade steps ------------------


bool DatabaseUpgrader::upgrade_1_to_2(QString &error)
{
    QSqlQuery q(m_db);

    if (!q.exec("SELECT value FROM app_info WHERE key='app_key'")) {
        error = q.lastError().text();
        return false;
    }

    if (!q.next()) {
        error = "app_key entry missing in app_info table.";
        return false;
    }

    QString value = q.value(0).toString().trimmed();

    //
    // Helpers
    //
    auto crc32 = [&](const QByteArray &data) -> quint32 {
        quint32 crc = 0xFFFFFFFF;
        for (unsigned char b : data) {
            crc ^= b;
            for (int i = 0; i < 8; ++i)
                crc = (crc >> 1) ^ (0xEDB88320 & (0u - (crc & 1u)));
        }
        return ~crc;
    };

    auto obfuscate = [&](const QByteArray &raw) -> QByteArray {
        QByteArray data = qCompress(raw);

        quint32 crc = crc32(data);
        data.append(char((crc >> 24) & 0xFF));
        data.append(char((crc >> 16) & 0xFF));
        data.append(char((crc >> 8) & 0xFF));
        data.append(char(crc & 0xFF));

        char xorKey = char(QRandomGenerator::global()->generate() & 0xFF);
        for (char &c : data)
            c ^= xorKey;

        data.prepend(xorKey);
        return data.toBase64();
    };

    QByteArray raw;

    // CASE 1: "exported" → user must paste key
    if (value.compare("exported", Qt::CaseInsensitive) == 0) {

        bool ok = false;
        QString keyText = QInputDialog::getMultiLineText(
            nullptr,
            QObject::tr("Database Upgrade"),
            QObject::tr("Paste your application key below for upgrade:"),
            QString(),
            &ok
            );

        if (!ok || keyText.trimmed().isEmpty()) {
            error = "Application key was not provided.";
            return false;
        }

        // Remove ALL whitespace
        keyText.remove(QRegularExpression("\\s+"));

        // Decode Base64 → raw hex string
        raw = QByteArray::fromBase64(keyText.toUtf8());

        if (raw.isEmpty()) {
            error = "The Application Key provided is invalid.";
            return false;
        }

        // VALIDATE raw key (must be 192 hex chars)
        QString decodedStr = QString::fromUtf8(raw);
        static const QRegularExpression re(QStringLiteral("^[A-Fa-f0-9]{192}$"));

        if (!re.match(decodedStr).hasMatch()) {
            error = "The Application Key provided is invalid, incomplete or incorrect.";
            return false;
        }
    }
    else
    {
        // CASE 2: old stored key → Base64(rawHexString)
        QByteArray stored = value.toUtf8();
        raw = QByteArray::fromBase64(stored);

        if (raw.isEmpty()) {
            error = "Failed to decode old-format application key (Base64).";
            return false;
        }

        // VALIDATE raw key (must be 192 hex chars)
        QString decodedStr = QString::fromUtf8(raw);
        static const QRegularExpression re(QStringLiteral("^[A-Fa-f0-9]{192}$"));

        if (!re.match(decodedStr).hasMatch()) {
            error = "Stored application key is invalid or corrupted.";
            return false;
        }
    }

    // Convert to NEW obfuscation format
    QByteArray newEncoded = obfuscate(raw);

    QSqlQuery update(m_db);
    update.prepare("UPDATE app_info SET value=? WHERE key='app_key'");
    update.addBindValue(QString::fromUtf8(newEncoded));

    if (!update.exec()) {
        error = update.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseUpgrader::upgrade_2_to_3(QString &error)
{
    return true;
}

bool DatabaseUpgrader::upgrade_3_to_4(QString &error)
{
    return true;
}
