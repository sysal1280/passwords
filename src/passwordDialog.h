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


#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace PasswordDialog {

class PasswordInspectorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PasswordInspectorDialog(const QString &password,
                                     QWidget *parent = nullptr);

private:
    // --- Internal helper struct ---
    struct PasswordAnalysis {
        int length = 0;
        int upperCount = 0;
        int lowerCount = 0;
        int digitCount = 0;
        int symbolCount = 0;
        double entropyBits = 0.0;
        QString entropyLabel;
        QString segmentation;
    };

    // --- Internal helper methods ---
    QWidget *makeLegendItem(const QString &color, const QString &label);
    QWidget *createCharacterTile(QChar c);
    QString characterType(QChar c) const;
    QString symbolName(QChar c) const;
    QString characterDescription(QChar c) const;

    PasswordAnalysis analyzePassword(const QString &password) const;
    QString segmentPassword(const QString &password) const;
    QString formatAnalysisSummary(const PasswordAnalysis &a) const;
};

// Existing generator function
void showPasswordGenerator(QWidget *parent,
                           const QString &title,
                           const QStringList &wordList);

} // namespace PasswordDialog

#endif // PASSWORDDIALOG_H
