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
