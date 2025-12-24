#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    QWidget* createIcon(QWidget *parent);
    QWidget* createAppName(QWidget *parent);
    QWidget* createVersion(QWidget *parent);
    QWidget* createCopyright(QWidget *parent);
    QWidget* createWebsite(QWidget *parent);
    QLayout* createCredits(QWidget *parent);
};

#endif // ABOUTDIALOG_H
