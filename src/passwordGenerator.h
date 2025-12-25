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


#ifndef PASSWORDGENERATOR_H
#define PASSWORDGENERATOR_H

#include <QString>
#include <QStringList>

class passwordGenerator {
public:
    // Load words from a text file into a QStringList
    static QStringList loadWordList(const QString &filePath);

    // Generate a password using the provided word list
    static QString generatePassword(const QStringList &wordList,
                                    int wordCount = 4,
                                    int maxNumber = 100);

private:
    // Helper: pick a random separator character
    static QChar randomSeparator();
};

#endif // PASSWORDGENERATOR_H
