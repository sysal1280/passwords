#ifndef TESTLOADER_H
#define TESTLOADER_H

#include <QObject>
#include <QDialog>
#include <QByteArray>
#include <QString>
#include <QSqlDatabase>
#include <QCloseEvent>

class QSpinBox;
class QProgressBar;
class QPushButton;

class TestLoader : public QObject
{
    Q_OBJECT
public:
    explicit TestLoader(const QString &dbFile,
                        const QByteArray &appKey,
                        QObject *parent = nullptr);

    bool generateCategories(int count, int maxDepth = 3);
    bool generateApplications(int count);
    QString getRandomGpgKey(const QString &dbFile, const QByteArray &appKey);

    void showDialog(QWidget *parent = nullptr);

private:
    QString dbFile;
    QByteArray appKey;

    bool openDb(QSqlDatabase &db, QString &connName);
    void closeDb(const QString &connName);

    int randomExistingCategoryId(QSqlDatabase &db);
    QString randomTextWithSpaces(int minLen, int maxLen);
    QString randomString(int length);
    QString fakeJson();
    QByteArray encryptWithGpg(const QStringList &recipients, const QString &json);

signals:
    void progress(int value);
};


// ---------------------------------------------------------
// Dialog class (must be OUTSIDE TestLoader for MOC to work)
// ---------------------------------------------------------

class TestLoaderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TestLoaderDialog(TestLoader *loader, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;

private slots:
    void onRunClicked();

private:
    TestLoader *loader;

    QSpinBox *categoryCountSpin;
    QSpinBox *categoryDepthSpin;
    QSpinBox *appCountSpin;

    QProgressBar *progressBar;
    QPushButton *runButton;
    QPushButton *closeButton;
};

#endif // TESTLOADER_H
