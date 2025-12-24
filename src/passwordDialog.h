#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QString>
#include <QStringList>
#include <QDialog>
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
