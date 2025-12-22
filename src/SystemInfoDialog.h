// systeminfodialog.h
#ifndef SYSTEMINFODIALOG_H
#define SYSTEMINFODIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QSysInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "settings.h"
#include "utils.h"

class SystemInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SystemInfoDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("System Information");
        resize(700, 500);

        auto *mainLayout = new QVBoxLayout(this);
        textEdit = new QPlainTextEdit(this);
        textEdit->setReadOnly(true);

        // Buttons row
        auto *buttonLayout = new QHBoxLayout;
        auto *copyBtn = new QPushButton("Copy to Clipboard", this);
        auto *closeBtn = new QPushButton("Close", this);

        buttonLayout->addStretch(); // push buttons to the right
        buttonLayout->addWidget(copyBtn);
        buttonLayout->addWidget(closeBtn);

        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(copyBtn, &QPushButton::clicked, this, [this]() {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(textEdit->toPlainText());
        });

        mainLayout->addWidget(textEdit);
        mainLayout->addLayout(buttonLayout);

        textEdit->setPlainText(collectSystemInfo());
    }

private:
    QPlainTextEdit *textEdit;
    Settings settings;

    QString collectSystemInfo()
    {
        QString report;

        // Qt version
        report += QString(tr("Qt Version: %1\n")).arg(qVersion());

        // OS info
        report += QString(tr("OS: %1\n")).arg(QSysInfo::prettyProductName());
        report += QString(tr("Kernel: %1 %2\n"))
                      .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());

        // Architecture
        report += QString(tr("Architecture: %1\n"))
                      .arg(QSysInfo::currentCpuArchitecture());

        // Environment variables
        report += tr("Environment:\n");
        auto env = QProcessEnvironment::systemEnvironment();
        const QStringList keys = env.keys();
        for (const QString &key : keys) {
            report += QString("  %1=\"%2\"\n").arg(key, env.value(key));
        }

        // Library paths
        report += tr("\nLibrary info:\n");
        report += QString("  PrefixPath: %1\n")
                      .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::PrefixPath)));
        report += QString("  PluginsPath: %1\n")
                      .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::PluginsPath)));
        report += QString("  TranslationsPath: %1\n")
                      .arg(QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::TranslationsPath)));

        // Standard paths
        report += tr("\nStandard paths:\n");
        auto locations = {
            QStandardPaths::DesktopLocation,
            QStandardPaths::DocumentsLocation,
            QStandardPaths::DownloadLocation,
            QStandardPaths::MusicLocation,
            QStandardPaths::PicturesLocation,
            QStandardPaths::MoviesLocation,
            QStandardPaths::HomeLocation,
            QStandardPaths::TempLocation
        };
        for (auto loc : locations) {
            QStringList paths = QStandardPaths::standardLocations(loc);
            for (QString &p : paths)
                p = QDir::toNativeSeparators(p);
            report += QString("  %1: %2\n")
                          .arg(QStandardPaths::displayName(loc),
                               paths.join(", "));
        }

        // Locations of program, conf and database
        report += QString(tr("  Configuration file: %1\n"
                          "  Database file: %2\n"
                             "  Executable file: %3\n"))
                      .arg(QDir::toNativeSeparators(settings.configFilePath()),
                           QDir::toNativeSeparators(qApp->property("dbFile").toString()),
                           QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));


        // AppKey source
            QString appKeyPath = appKeyFilePath();
            if (!appKeyPath.isEmpty() && QFile::exists(appKeyPath)) {
                report += QString("\nAppKey:\n  Sourced from local file (%1)\n")
                .arg(QDir::toNativeSeparators(appKeyPath));
            } else {
                report += "\nAppKey:\n  Sourced from database\n";
            }

        // GPG info
        QString gpgPath = QStandardPaths::findExecutable("gpg");
        if (!gpgPath.isEmpty()) {
            report += QString(tr("\nGPG Path: %1\n")).arg(QDir::toNativeSeparators(gpgPath));

            QProcess proc;
            proc.start(gpgPath, {"--version"});
            proc.waitForFinished(2000);
            QString gpgOutput = proc.readAllStandardOutput();

            if (!gpgOutput.isEmpty()) {
                report += tr("GPG Version and Defaults:\n");
                report += gpgOutput + "\n";

                // Optional: extract default cipher/hash lines
                QStringList lines = gpgOutput.split('\n');
                for (const QString &line : std::as_const(lines)) {
                    if (line.startsWith("Default")) {
                        report += "  " + line + "\n";
                    }
                }
            }
        } else {
            report += tr("\nGPG not found in PATH.\n");
        }

        // Show gpg.conf contents
        QString gpgConfPath = QDir::toNativeSeparators(QDir::homePath() + "/.gnupg/gpg.conf");
        QFile confFile(gpgConfPath);
        if (confFile.exists() && confFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            report += QString(tr("User GPG configuration (%1):\n")).arg(gpgConfPath);
            QTextStream in(&confFile);
            while (!in.atEnd()) {
                QString line = in.readLine();
                report += "  " + line + "\n";
            }
        } else {
            report += QString(tr("No gpg.conf found at %1\n")).arg(gpgConfPath);
        }

        return report;
    }
};

#endif // SYSTEMINFODIALOG_H
