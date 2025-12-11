#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QMap>
#include <QAbstractButton>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();
    void loadSettings();
    void saveSettings();

private slots:
    void on_pushButton_clicked();    

private:
    Ui::PreferencesDialog *ui;
    QMap<QString, QWidget*> widgetMap;
    void resizeEvent(QResizeEvent *event);
    void restoreButtonClicked(QAbstractButton *button);
    void restoreDefaults();
    void onBackupCheckStateChanged(int state);

};

#endif // PREFERENCESDIALOG_H
