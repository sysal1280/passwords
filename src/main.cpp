/*
 * passwords - A simple password manager
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

#include "mainwindow.h"
#include "gpgCheck.h"
#include "logindialog.h"
#include "settings.h"
#include <QApplication>
#include <QPixmap>
#include <QSplashScreen>
#include <QTimer>
#include <QStyleFactory>
#include <QDebug>
#include <QMessageBox>
#include <QLockFile>
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QFileDevice>
#include <QStandardPaths>


// Forward declaration of helper
// Returns true if a database file is ready (exists or was created/copied)
// and qApp->property("dbFile") is set. It DOES NOT open the DB.
static bool setupDatabaseFile();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /*
     * Set QApplication settings
     */
    QString applicationName = QCoreApplication::translate("main","passwords");
    QApplication::setWindowIcon(QIcon(":/password.png"));
    QGuiApplication::setDesktopFileName(applicationName);
    QApplication::setApplicationName(applicationName);
    QApplication::setApplicationVersion("0.1.0");
    if (auto *style = QStyleFactory::create("Fusion")) {
        QApplication::setStyle(style);
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
        QMessageBox::warning(nullptr,QApplication::applicationDisplayName(),
                             QCoreApplication::translate("main", "Already running."));
        return 0;
    }


    QApplication::setOverrideCursor(Qt::WaitCursor);

    QPixmap pixmap(":/splash.png");
    QSplashScreen *splash = new QSplashScreen(pixmap);
    splash->showMessage(QCoreApplication::translate("main", "Setting language.."),
                        Qt::AlignBottom | Qt::AlignLeft,Qt::white);
    splash->show();

    QApplication::processEvents();


    /*
     * Set Translation
     */
    QTranslator translator;
    QString locale = QLocale::system().name().toLower().trimmed();

    if (translator.load(locale, QCoreApplication::applicationDirPath())) {
        QApplication::installTranslator(&translator);
    } else {
        qWarning().noquote()
        << QString(QCoreApplication::translate("main", "No translation file found for %1, using defaults.")).arg(locale);
        qWarning().noquote()
            << QString(QCoreApplication::translate("main", "To load translation add %1.qm file to %2"))
                   .arg(locale, QCoreApplication::applicationDirPath());
    }


    /*
     * Create a shortcut in Linux, if application
     * is in an installed path
     */
    splash->showMessage(QCoreApplication::translate("main", "Checking installation.."),
                        Qt::AlignBottom | Qt::AlignLeft,Qt::white);
    QApplication::processEvents();
    Settings::createUserDesktopFile();


    /*
     * init settings for default config file
     */
    Settings();

    splash->showMessage(QCoreApplication::translate("main", "Checking GPG.."),
                        Qt::AlignBottom | Qt::AlignLeft, Qt::white);
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
                        Qt::AlignBottom | Qt::AlignLeft,Qt::white);
    QApplication::processEvents();

    QString stylePath = QCoreApplication::applicationDirPath() + "/style.css";
    QFile styleFile(QFile::exists(stylePath) ? stylePath : QString(":/files/style.css"));

    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));

        if (styleFile.fileName().startsWith(":/")) {
            qWarning().noquote()
            << QString(QCoreApplication::translate("main", "No stylesheet file found, using defaults."));
            qWarning().noquote()
                << QString(QCoreApplication::translate("main", "To load stylesheet add style.css file to %1"))
                       .arg(QCoreApplication::applicationDirPath());
        } else {
            qInfo().noquote()
            << QString(QCoreApplication::translate("main", "Loaded stylesheet file: %1")).arg(styleFile.fileName());
        }
    }


    /*
     * Prepare database file BEFORE GPG checks
     * Note: this only ensures the file exists and qApp->dbFile is set.
     * Opening is done by MainWindow since initDb/openDatabase are its methods.
     */
    splash->showMessage(QCoreApplication::translate("main", "Setting up database file.."),
                        Qt::AlignBottom | Qt::AlignLeft, Qt::white);
    QApplication::processEvents();
    bool dbFileReady = setupDatabaseFile();

    /*
     * Create main window (needed to call its DB methods)
     */
    splash->showMessage(QCoreApplication::translate("main", "Loading main window"),
                        Qt::AlignBottom | Qt::AlignLeft, Qt::white);
    QApplication::processEvents();
    MainWindow *w = new MainWindow;
    w->setWindowTitle(QApplication::applicationDisplayName());
    w->setWindowIcon(QIcon(":/password.png"));

    /*
     * Open database in MainWindow if file is ready
     */
    bool dbOpened = false;
    if (dbFileReady) {
        const QString dbPath = qApp->property("dbFile").toString();
        if (!dbPath.isEmpty()) {
            // Call member methods on MainWindow
            w->initDb();
            if (!w->openDatabase(dbPath))
                return 0;
            dbOpened = true;
        }
    } else {
        qWarning() << QCoreApplication::translate("main", "Database file not ready, running in limited mode.");
    }


    /*
     * GPG key checking — only if DB is opened
     */
    if (dbOpened) {

        splash->showMessage(QCoreApplication::translate("main", "Checking keys.."),
                            Qt::AlignBottom | Qt::AlignLeft, Qt::white);
        QApplication::processEvents();
        checkGpgKeys(splash);
    }

    /*
     * Show main window with slight delay and handle login preference
     */
    QTimer::singleShot(500, w, [w, splash]() {
        w->show();
        w->activateWindow();
        w->raise();
        QApplication::processEvents(QEventLoop::AllEvents, 100);

        w->setEnabled(false);

        QApplication::restoreOverrideCursor();
        QApplication::processEvents();

        splash->finish(w);
        delete splash;

        if (Settings::getLoginPreference()) {
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
static bool setupDatabaseFile()
{
    qApp->setProperty("dbFile", Settings::getDefaultDbPath(nullptr));

    const QString dbPath = qApp->property("dbFile").toString();
    QFile file(dbPath);

    if (file.exists()) {
        return true;
    }

    qDebug() << "No db found at" << dbPath;

    QMessageBox::StandardButton reply =
        QMessageBox::question(
            nullptr,
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

        qDebug() << "Creating database at" << localPath;

        if (QFile::copy(":/files/passwords", localPath)) {
            QFile::setPermissions(localPath,
                                  QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            qApp->setProperty("dbFile", localPath);
            return true;
        } else {
            QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("main", "Error"),
                QCoreApplication::translate("main", "Failed to copy database template to %1")
                    .arg(localPath)
                );
            return false;
        }
    } else {
        QMessageBox::information(
            nullptr,
            QCoreApplication::translate("main", "Database Required"),
            QCoreApplication::translate("main",
                                        "The application can continue without a database,\n"
                                        "but functionality will be severely limited.\n"
                                        "For example, you won't be able to save passwords.")
            );
        return false;
    }
}
