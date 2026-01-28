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


#include "passwordgenerator.h"

#include <QFile>
#include <QRandomGenerator>
#include <QTextStream>

#include <algorithm>


QStringList passwordGenerator::loadWordList(const QString &filePath) {
    QFile file(filePath);
    QStringList words;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty())
                words.append(line);
        }
    }
    return words;
}

QString capitalizeWord(const QString &word) {
    if (word.isEmpty())
        return word;

    QString cap = word;
    cap.replace(0, 1, cap.at(0).toUpper());
    return cap;
}

QString applySmartSymbolSubstitution(const QString &word, bool &substitutionUsed) {

    if (substitutionUsed)
        return word;

    // 40% chance to apply substitution at all
    if (QRandomGenerator::global()->bounded(100) >= 40)
        return word;

    QString w = word;
    QList<int> candidatePositions;

    // Identify letters that have meaningful symbol replacements
    for (int i = 0; i < w.length(); ++i) {
        QChar c = w[i].toLower();
        if (c == 'a' || c == 's' || c == 'i' || c == 'o' || c == 'e' || c == 't')
            candidatePositions.append(i);
    }

    if (candidatePositions.isEmpty())
        return word;

    // Pick one position to replace
    int pos = candidatePositions.at(
        QRandomGenerator::global()->bounded(candidatePositions.size())
        );

    QChar c = w[pos].toLower();
    QChar replacement;

    switch (c.unicode()) {
    case 'a': replacement = '@'; break;
    case 's': replacement = '$'; break;
    case 'i': replacement = '!'; break;
    case 'o': replacement = '0'; break;
    case 'e': replacement = '3'; break;
    case 't': replacement = '+'; break;
    default: return word;
    }

    w[pos] = replacement;

    // Mark substitution as used
    substitutionUsed = true;

    return w;
}

QString passwordGenerator::generatePassword(const QStringList &wordList,
                                            int wordCount,
                                            int maxNumber) {
    if (wordList.isEmpty())
        return QString("ERROR: Word list is empty");

    QStringList shuffled = wordList;
    std::shuffle(shuffled.begin(), shuffled.end(), *QRandomGenerator::global());

    // Ensure we have enough words
    if (shuffled.size() < wordCount)
        return QString("ERROR: Word list too small");

    QStringList chosenWords;
    bool substitutionUsed = false;

    // Pick first N unique words
    for (int i = 0; i < wordCount; ++i) {
        QString w = capitalizeWord(shuffled.at(i));

        // Apply smart symbol substitution (only once per password)
        w = applySmartSymbolSubstitution(w, substitutionUsed);

        chosenWords << w;
    }

    int numWordIndex = QRandomGenerator::global()->bounded(chosenWords.size());
    int number = QRandomGenerator::global()->bounded(qMax(1, maxNumber));

    bool placeAtBeginning = QRandomGenerator::global()->bounded(2); // 0 or 1

    if (placeAtBeginning)
        chosenWords[numWordIndex] = QString::number(number) + chosenWords[numWordIndex];
    else
        chosenWords[numWordIndex] += QString::number(number);

    QString password;
    for (int i = 0; i < chosenWords.size(); ++i) {
        password += chosenWords[i];
        if (i < chosenWords.size() - 1) {
            password += randomSeparator();
        }
    }

    return password;
}

QChar passwordGenerator::randomSeparator() {
    static QList<QChar> shuffled;
    static int index = 0;

    // Base separator list
    static const QList<QChar> baseList = {
        '-', '_', ':', '.', '!', '@', '$', '^', '&', '*', '~', '?'
    };

    // Refill + reshuffle when needed
    if (shuffled.isEmpty() || index >= shuffled.size()) {
        shuffled = baseList;
        std::shuffle(shuffled.begin(), shuffled.end(), *QRandomGenerator::global());
        index = 0;
    }

    return shuffled[index++];
}
