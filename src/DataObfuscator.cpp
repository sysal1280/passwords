#include "DataObfuscator.h"
#include <QRandomGenerator64>
#include <QCryptographicHash>

QByteArray DataObfuscator::xorProcess(const QByteArray &data, const QByteArray &key)
{
    if (key.isEmpty())
        return data;

    QByteArray result;
    result.resize(data.size());

    QByteArray keystream;
    // Expand key into a long pseudo‑random stream
    for (int block = 0; keystream.size() < data.size(); ++block) {
        QByteArray material = key + QByteArray::number(block);
        keystream.append(QCryptographicHash::hash(material, QCryptographicHash::Sha256));
    }

    for (int i = 0; i < data.size(); ++i)
        result[i] = data[i] ^ keystream[i];

    return result;
}

QString DataObfuscator::obfuscate(const QString &data, const QByteArray &key)
{
    QByteArray raw = data.toUtf8();
    QByteArray salt(16, Qt::Uninitialized);
    QRandomGenerator::system()->generate(salt.begin(), salt.end());

    QByteArray xored = xorProcess(raw, key + salt);
    return QString((salt + xored).toBase64());
}

QString DataObfuscator::deobfuscate(const QString &encoded, const QByteArray &key)
{
    QByteArray decoded = QByteArray::fromBase64(encoded.toUtf8());
    QByteArray salt = decoded.left(16);
    QByteArray payload = decoded.mid(16);

    QByteArray xored = xorProcess(payload, key + salt);
    return QString::fromUtf8(xored);
}
