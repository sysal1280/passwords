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


#ifndef DATAOBFUSCATOR_H
#define DATAOBFUSCATOR_H

#include <QString>
#include <QByteArray>

/**
 * @brief The DataObfuscator class
 *
 * Provides lightweight reversible obfuscation using a simple XOR algorithm.
 *
 * This is NOT cryptographically secure — it's meant only to deter casual
 * inspection of stored data (e.g., in SQLite or config files).
 *
 * Usage:
 *
 *     QString enc = DataObfuscator::obfuscate("secret text", "key123");
 *     QString dec = DataObfuscator::deobfuscate(enc, "key123");
 */
class DataObfuscator
{
public:
    /**
     * @brief Obfuscate a string using a simple XOR-based reversible algorithm.
     * @param data The plaintext to obfuscate.
     * @param key The key used for XOR (any non-empty QByteArray).
     * @return Base64-encoded obfuscated string.
     */
    static QString obfuscate(const QString &data, const QByteArray &key);

    /**
     * @brief Reverse the obfuscation.
     * @param encoded The Base64-encoded obfuscated string.
     * @param key The same key used for obfuscation.
     * @return The original plaintext.
     */
    static QString deobfuscate(const QString &encoded, const QByteArray &key);

private:
    // Internal helper performing XOR on two byte arrays.
    static QByteArray xorProcess(const QByteArray &data, const QByteArray &key);
};

#endif // DATAOBFUSCATOR_H
