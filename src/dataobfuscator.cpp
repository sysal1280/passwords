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


#include "dataobfuscator.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QtGlobal>


static const int SALT_SIZE = 16;
static const int MAC_SIZE  = 32;

static QByteArray deriveKey(const QByteArray &key, const QByteArray &info)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(key);
    hash.addData(info);
    return hash.result();
}

static QByteArray hmacSha256(const QByteArray &key, const QByteArray &data)
{
    QByteArray k = key;
    const int blockSize = 64;

    if (k.size() > blockSize) {
        k = QCryptographicHash::hash(k, QCryptographicHash::Sha256);
    }
    if (k.size() < blockSize) {
        k.append(QByteArray(blockSize - k.size(), '\0'));
    }

    QByteArray o_key_pad(blockSize, '\0');
    QByteArray i_key_pad(blockSize, '\0');

    for (int i = 0; i < blockSize; ++i) {
        o_key_pad[i] = static_cast<char>(k[i] ^ 0x5c);
        i_key_pad[i] = static_cast<char>(k[i] ^ 0x36);
    }

    // inner = H((K ^ ipad) || data)
    QCryptographicHash innerHash(QCryptographicHash::Sha256);
    innerHash.addData(i_key_pad);
    innerHash.addData(data);
    QByteArray inner = innerHash.result();

    // outer = H((K ^ opad) || inner)
    QCryptographicHash outerHash(QCryptographicHash::Sha256);
    outerHash.addData(o_key_pad);
    outerHash.addData(inner);
    return outerHash.result();
}

static QByteArray streamXor(const QByteArray &data,
                            const QByteArray &encKey,
                            const QByteArray &salt)
{
    QByteArray result;
    result.resize(data.size());

    QByteArray keystream;
    keystream.reserve(data.size());

    int blockIndex = 0;

    while (keystream.size() < data.size()) {
        QByteArray material;
        material.reserve(encKey.size() + salt.size() + 4);

        material.append(encKey);
        material.append(salt);

        quint32 counter = static_cast<quint32>(blockIndex);
        char ctr[4];
        ctr[0] = static_cast<char>((counter >> 24) & 0xFF);
        ctr[1] = static_cast<char>((counter >> 16) & 0xFF);
        ctr[2] = static_cast<char>((counter >> 8) & 0xFF);
        ctr[3] = static_cast<char>(counter & 0xFF);
        material.append(ctr, 4);

        QByteArray block =
            QCryptographicHash::hash(material, QCryptographicHash::Sha256);

        keystream.append(block);
        ++blockIndex;
    }

    for (int i = 0; i < data.size(); ++i) {
        result[i] = static_cast<char>(data[i] ^ keystream[i]);
    }

    return result;
}

static bool constantTimeEqual(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size())
        return false;

    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

QString DataObfuscator::obfuscate(const QString &data, const QByteArray &key)
{
    if (key.isEmpty())
        return data;

    QByteArray raw = data.toUtf8();

    // Derive separate keys for encryption and MAC
    QByteArray encKey = deriveKey(key, QByteArrayLiteral("enc"));
    QByteArray macKey = deriveKey(key, QByteArrayLiteral("mac"));

    QByteArray salt(SALT_SIZE, Qt::Uninitialized);
    for (int i = 0; i < SALT_SIZE; ++i)
        salt[i] = static_cast<char>(QRandomGenerator::system()->generate() & 0xFF);

    QByteArray ciphertext = streamXor(raw, encKey, salt);

    QByteArray macInput;
    macInput.reserve(salt.size() + ciphertext.size());
    macInput.append(salt);
    macInput.append(ciphertext);

    QByteArray mac = hmacSha256(macKey, macInput);

    QByteArray out;
    out.reserve(salt.size() + ciphertext.size() + mac.size());
    out.append(salt);
    out.append(ciphertext);
    out.append(mac);

    return QString::fromLatin1(out.toBase64());
}

QString DataObfuscator::deobfuscate(const QString &encoded, const QByteArray &key)
{
    if (key.isEmpty())
        return encoded;

    QByteArray decoded = QByteArray::fromBase64(encoded.toLatin1());
    if (decoded.size() < SALT_SIZE + MAC_SIZE) {
        return QString();
    }

    QByteArray salt = decoded.left(SALT_SIZE);
    QByteArray mac = decoded.right(MAC_SIZE);
    QByteArray ciphertext = decoded.mid(SALT_SIZE,
                                        decoded.size() - SALT_SIZE - MAC_SIZE);

    QByteArray encKey = deriveKey(key, QByteArrayLiteral("enc"));
    QByteArray macKey = deriveKey(key, QByteArrayLiteral("mac"));

    QByteArray macInput;
    macInput.reserve(salt.size() + ciphertext.size());
    macInput.append(salt);
    macInput.append(ciphertext);

    QByteArray expectedMac = hmacSha256(macKey, macInput);

    if (!constantTimeEqual(mac, expectedMac)) {
        // MAC mismatch - tampered or wrong key
        return QString();
    }

    QByteArray plaintext = streamXor(ciphertext, encKey, salt);
    return QString::fromUtf8(plaintext);
}
