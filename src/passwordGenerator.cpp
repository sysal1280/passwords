#include "passwordGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QRandomGenerator>

// Load words from a file (one word per line)
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
    if (word.isEmpty()) return word;
    QString cap = word;
    cap[0] = cap[0].toUpper();
    return cap;
}

QString passwordGenerator::generatePassword(const QStringList &wordList,
                                            int wordCount,
                                            int maxNumber) {
    if (wordList.isEmpty())
        return QString("ERROR: Word list is empty");

    const QString symbols = "!@#$%^&*()-_=+[]{};:,.<>?";
    QStringList chosenWords;

    // Pick random words
    for (int i = 0; i < wordCount; ++i) {
        int index = QRandomGenerator::global()->bounded(wordList.size());
        chosenWords << capitalizeWord(wordList.at(index));
    }

    // Choose random positions for number and symbol
    int numWordIndex = QRandomGenerator::global()->bounded(chosenWords.size());
    int symWordIndex = QRandomGenerator::global()->bounded(chosenWords.size());

    // Append number to one word
    chosenWords[numWordIndex] += QString::number(QRandomGenerator::global()->bounded(maxNumber));

    // Append symbol to another word (could be same word, that’s fine)
    chosenWords[symWordIndex] += symbols.at(QRandomGenerator::global()->bounded(symbols.size()));

    // Join words with random separators
    QString password;
    for (int i = 0; i < chosenWords.size(); ++i) {
        password += chosenWords[i];
        if (i < chosenWords.size() - 1) {
            password += randomSeparator();
        }
    }

    return password;
}

// Pick a random separator character
QChar passwordGenerator::randomSeparator() {
    const QString separators = "-_:.!@$^&*";
    int sepIndex = QRandomGenerator::global()->bounded(separators.size());
    return separators.at(sepIndex);
}

