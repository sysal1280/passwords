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


#include "constants.h"
#include "debugutils.h"
#include "gpgcheck.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "settings.h"
#include "termsdialog.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QLocale>
#include <QLockFile>
#include <QMessageBox>
#include <QPixmap>
#include <QSplashScreen>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTimer>
#include <QTranslator>

// Forward declaration of helper
// Returns true if a database file is ready (exists or was created/copied)
// and qApp->property("dbFile") is set. It DOES NOT open the DB.
static bool setupDatabaseFile(QWidget *parent);
void pwdMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
static bool showDebugMessages = false;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (isDebuggerAttached()) {
        return 0;
    }

    /*
     * Set QApplication settings
     */
    QString translatedName =
        QCoreApplication::translate("main", Passwords::Name);

    QApplication::setWindowIcon(QIcon(Passwords::Icon));
    QGuiApplication::setDesktopFileName(translatedName);
    QApplication::setApplicationName(translatedName);
    QApplication::setApplicationVersion(Passwords::Version);
    QApplication::setOrganizationName(Passwords::Organization);

    if (auto *style = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(style);
    }

    /*
     * init settings for default config file
     */
    Settings settings;
    showDebugMessages = settings.getDebugMode();

    qInstallMessageHandler(pwdMsgHandler);
    {
        Settings settings;
        qInfo().noquote() << "Config file loaded:" << QDir::toNativeSeparators(settings.configFilePath());

    }

    /*
     * Check for multiple instances.
     */
    QString lockFileName = QString("%1-%2.lock")
                               .arg(QCoreApplication::applicationName(),
                                    QCoreApplication::applicationVersion());
    QLockFile lockFile(QDir::temp().absoluteFilePath(lockFileName));
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock()) {
        qWarning() << "Already running.";
        QMessageBox::warning(nullptr,QApplication::applicationDisplayName(),
                             QCoreApplication::translate("main", "Already running."));
        return 0;
    }


    QApplication::setOverrideCursor(Qt::WaitCursor);

    /*
     * custom or default splash
     */
    QPixmap pixmap(QFile::exists("splash.png")
                       ? "splash.png"
                       : ":/splash.png");
    QSplashScreen *splash = new QSplashScreen(pixmap);
    splash->showMessage(QCoreApplication::translate("main", "Setting language.."),
                        Qt::AlignBottom | Qt::AlignLeft,QColor(0x21, 0x4d, 0x68));
    splash->show();

    QApplication::processEvents();


    /*
     * Set Translation
     */
    QTranslator translator;
    QString locale = QLocale::system().name().toLower().trimmed();

    if (translator.load(locale, QCoreApplication::applicationDirPath())) {
        QApplication::installTranslator(&translator);
        qInfo().noquote() << QString(QCoreApplication::translate("main", "Loaded translation file: %1.")).arg(locale+".qm");
    } else {
        qWarning().noquote() << QString(QCoreApplication::translate("main", "No translation file found for %1, using defaults.")).arg(locale);
        qInfo().noquote() << QString(QCoreApplication::translate("main", "To load translation add %1.qm file to %2.")).arg(locale, QCoreApplication::applicationDirPath());
    }


    /*
     * Create a shortcut in Linux, if application
     * is in an installed path
     */
    splash->showMessage(QCoreApplication::translate("main", "Checking installation.."),
                        Qt::AlignBottom | Qt::AlignLeft,QColor(0x21, 0x4d, 0x68));
    QApplication::processEvents();
    settings.createUserDesktopFile();

    splash->showMessage(QCoreApplication::translate("main", "Checking GPG.."),
                        Qt::AlignBottom | Qt::AlignLeft, QColor(0x21, 0x4d, 0x68));
    QApplication::processEvents();


    /*
     * GPG tool checking
     */

    if (Q_UNLIKELY(!isToolAvailable("gpg")))
    {
        QApplication::restoreOverrideCursor();
        QApplication::processEvents();
        QString msg;

#ifdef Q_OS_WIN
        msg = QCoreApplication::translate("main", R"(
The GPG tool is either not installed or cannot be found.

Please install Gpg4win (https://gpg4win.org) and ensure
the installation directory is added to your PATH environment variable.
)");
#elif defined(Q_OS_MAC)
        msg = QCoreApplication::translate("main", R"(
The GPG tool is either not installed or cannot be found.

You can install GPG via Homebrew:

<pre>
    brew install gnupg
</pre>

Make sure your PATH includes Homebrew’s bin directory.
)");
#else
        msg = QCoreApplication::translate("main", R"(
The GPG tool is either not installed or cannot be found.

Please install GPG using your distribution’s package manager, e.g.

<pre>
sudo apt install gnupg      (Debian/Ubuntu)
sudo dnf install gnupg      (Fedora)
sudo pacman -S gnupg        (Arch)
sudo zypper install gnupg   (openSUSE)
</pre>

Ensure it is accessible in your PATH.
)");
#endif
        qFatal() << msg;
        QMessageBox box(QMessageBox::Critical,
                        QApplication::applicationDisplayName(),
                        msg,
                        QMessageBox::Ok);
        box.setTextFormat(Qt::RichText);   // interpret HTML
        box.exec();

        return 0;
    }

    /*
     * Load stylesheet
     */

    splash->showMessage(QCoreApplication::translate("main", "Setting style.."),
                        Qt::AlignBottom | Qt::AlignLeft,QColor(0x21, 0x4d, 0x68));
    QApplication::processEvents();

    QString stylePath = QCoreApplication::applicationDirPath() + "/style.css";

    // populate empty style.css with internal default if present.
    // allows developers and power users to easily tinker with styling.
    if (QFile::exists(stylePath)) {
        QFile checkFile(stylePath);
        if (checkFile.open(QFile::ReadOnly | QFile::Text)) {
            QString content = QString::fromUtf8(checkFile.readAll());
            checkFile.close();

            if (content.trimmed().isEmpty()) {
                QFile internalFile(":/files/style.css");
                if (internalFile.open(QFile::ReadOnly | QFile::Text)) {
                    if (checkFile.open(QFile::WriteOnly | QFile::Text)) {
                        checkFile.write(internalFile.readAll());
                        checkFile.close();
                        qInfo().noquote() << "Populated empty style.css with default internal stylesheet.";
                    } else {
                        qWarning().noquote() << "Failed to write to style.css (insufficient permissions?).";
                    }
                } else {
                    qWarning().noquote() << "Failed to open internal stylesheet resource.";
                }
            }
        }
    }

    QFile styleFile(QFile::exists(stylePath) ? stylePath : QString(":/files/style.css"));

    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));

        if (styleFile.fileName().startsWith(":/")) {
            qWarning().noquote() << QString(QCoreApplication::translate("main", "No stylesheet file found, using defaults."));
            qInfo().noquote() << QString(QCoreApplication::translate("main", "To load stylesheet add style.css file to %1.")).arg(QCoreApplication::applicationDirPath());
        } else {
            qInfo().noquote() << QString(QCoreApplication::translate("main", "Loaded stylesheet file: %1.")).arg(styleFile.fileName());
        }
    }


    /*
     * Prepare database file BEFORE GPG checks
     * Note: this only ensures the file exists and qApp->dbFile is set.
     * Opening is done by MainWindow since initDb/openDatabase are its methods.
     */
    splash->showMessage(QCoreApplication::translate("main", "Setting up database file.."),
                        Qt::AlignBottom | Qt::AlignLeft, QColor(0x21, 0x4d, 0x68));
    QApplication::processEvents();
    bool dbFileReady = setupDatabaseFile(splash);

    /*
     * Create main window (needed to call its DB methods)
     */
    splash->showMessage(QCoreApplication::translate("main", "Loading main window"),
                        Qt::AlignBottom | Qt::AlignLeft, QColor(0x21, 0x4d, 0x68));
    QApplication::processEvents();
    MainWindow *w = new MainWindow;
    w->setWindowTitle(QApplication::applicationDisplayName());
    w->setWindowIcon(QIcon(":/password.png"));

    /*
     * Open database in MainWindow if file is ready
     */
    w->abortingStartup = true;
    bool dbOpened = false;
    if (dbFileReady) {
        const QString dbPath = qApp->property("dbFile").toString();
        if (!dbPath.isEmpty()) {
            // Call member methods on MainWindow
            if (splash && splash->isVisible()) {
                splash->hide();
            }
            if (!w->initDb()) {
                delete w;
                return 0;   // abort cleanly before event loop
            }
            if (!w->openDatabase(dbPath))
            {
                delete w;
                return 0;
            }
            dbOpened = true;
        }
    }
    if (splash) {
        splash->show();
        splash->raise();
    }

    /*
     * Terms - optional dialog
     */
    QString termsPath = QCoreApplication::applicationDirPath() + "/conditions.txt";

    if (Q_UNLIKELY(QFile::exists(termsPath))) {
        QFile f(termsPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (splash && splash->isVisible()) {
                splash->hide();
            }
            QString termsText = f.readAll();

            TermsDialog dlg(termsText,splash);
            if (dlg.exec() != QDialog::Accepted)
                return 0;   // User declined
        }
    }
    if (splash) {
        splash->show();
        splash->raise();
    }

    w->abortingStartup = false;


    /*
     * GPG key checking — only if DB is opened
     */
    if (dbOpened) {

        splash->showMessage(QCoreApplication::translate("main", "Checking keys.."),
                            Qt::AlignBottom | Qt::AlignLeft, QColor(0x21, 0x4d, 0x68));
        QApplication::processEvents();

        checkGpgKeys(splash);
    }

    /*
     * Show main window with slight delay and handle login preference
     */
    QTimer::singleShot(500, w, [w, splash, &settings]() {
        w->show();
        w->activateWindow();
        w->raise();
        QApplication::processEvents(QEventLoop::AllEvents, 100);

        w->setEnabled(false);

        QApplication::restoreOverrideCursor();
        QApplication::processEvents();

        splash->finish(w);
        delete splash;

        if (settings.getLoginPreference()) {
            qInfo().noquote() << "Issuing access challenge.";
            LoginDialog login(w);
            login.move(w->geometry().center() - login.rect().center());
            login.setModal(true);

            if (!login.hasKeys() || login.exec() == QDialog::Accepted) {
                w->setEnabled(true);
            } else {
                QCoreApplication::exit(0);
            }
        } else {
            w->setEnabled(true);
        }
    });

    return a.exec();
}

/*
 * Helper: ensure a database file exists and set qApp->dbFile.
 * Does NOT open the database (that is done by MainWindow).
 */
static bool setupDatabaseFile(QWidget *parent)
{
    Settings settings;
    qApp->setProperty("dbFile", settings.getDefaultDbPath(nullptr));

    const QString dbPath = qApp->property("dbFile").toString();
    QFile file(dbPath);

    if (file.exists()) {
        qInfo().noquote() << QString("Found database %1.").arg(dbPath);
        return true;
    }

    qWarning().noquote() << "No database has been found.";

    QMessageBox::StandardButton reply =
        QMessageBox::question(
            parent,
            QCoreApplication::translate("main", "Database Missing"),
            QCoreApplication::translate("main",
                                        "No database was found.\n"
                                        "Would you like to create a new one?"),
            QMessageBox::Yes | QMessageBox::No
            );

    if (reply == QMessageBox::Yes) {
        QString localDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir(localDir).mkpath(".");  // ensure directory exists

        QString localPath = QDir(localDir).filePath("passwords");

        // Ensure parent directory exists
        QDir(QFileInfo(localPath).absolutePath()).mkpath(".");

        qInfo().noquote() << QString("Creating database at %1.").arg(localPath);

        if (QFile::copy(":/files/passwords", localPath)) {
            QFile::setPermissions(localPath,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            qApp->setProperty("dbFile", localPath);
            return true;
        } else {
            qCritical().noquote() << "Failed to copy database template to %1";
            QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("main", "Error"),
                QCoreApplication::translate("main", "Failed to copy database template to %1")
                    .arg(localPath)
                );
            return false;
        }
    } else {
        qInfo().noquote() << "Running in limited mode. A database should be setup for full functionality.";
        QMessageBox::information(
            parent,
            QCoreApplication::translate("main", "Database Required"),
            QCoreApplication::translate("main",
                                        "%1 can continue without a database,\n"
                                        "but functionality will be severely limited.\n"
                                        "For example, you won't be able to create passwords.").arg(QApplication::applicationName())
            );
        return false;
    }
}

void pwdMsgHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (!showDebugMessages)
        return;

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QByteArray localMsg = msg.toLocal8Bit();
    const char* prefix;

    switch (type) {
    case QtDebugMsg:   prefix = "[DEBUG] "; break;
    case QtInfoMsg:    prefix = "[INFO]  "; break;
    case QtWarningMsg: prefix = "[WARN]  "; break;
    case QtCriticalMsg:prefix = "[ERROR] "; break;
    case QtFatalMsg:   prefix = "[FATAL] "; break;
    }

    fprintf(stderr, "%s [%s] %s\n",
            prefix,
            timestamp.toLocal8Bit().constData(),
            localMsg.constData());
    fflush(stderr);
}
