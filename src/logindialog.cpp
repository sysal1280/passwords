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

#include "logindialog.h"
#include "ui_logindialog.h"
#include "mainwindow.h"
#include "settings.h"
#include "DataObfuscator.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QtGuiDepends>
#include <QMessageBox>
#include <QPushButton>
#include <QClipboard>
#include <QPixmap>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Login"));
    ui->textEdit->setReadOnly(true);

    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false); // disable initially

    QString connectionName = QUuid::createUuid().toString();
        // create a scoped database object
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(qApp->property("dbFile").toString());
            if (db.open()) {
                QSqlQuery query(db);
                query.setForwardOnly(true);
                query.prepare("SELECT label,key FROM keys ORDER BY label");
                if (query.exec()) {
                    ui->comboBoxLogin->clear();
                    keys.clear();

                    while (query.next()) {
                        const QString label = DataObfuscator::deobfuscate(query.value(0).toString(), qApp->property("appKey").toByteArray());
                        const QString key   = DataObfuscator::deobfuscate(query.value(1).toString(), qApp->property("appKey").toByteArray());
                        ui->comboBoxLogin->addItem(label);
                        keys.insert(label, key);
                    }

                    // --- activate first entry and update clipboard ---
                    if (ui->comboBoxLogin->count() > 0) {
                        ui->comboBoxLogin->setCurrentIndex(0);
                        generateResponse();
                    }
                } else
                {
                    qCritical().noquote() << Q_FUNC_INFO << "Query Failed." << query.lastError().text();
                }
                db.close();
            } else {
                qCritical().noquote() << Q_FUNC_INFO << "No open database." << db.lastError().text();
                QMessageBox::critical(this, tr("Database Error"),
                                      tr("No open database.\n%1").arg(db.lastError().text()));
            }
        }
        // remove connection after db object is destroyed
        QSqlDatabase::removeDatabase(connectionName);

    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, this, [=]() {
        if (auto mw = qobject_cast<MainWindow*>(parentWidget())) {
            mw->launchHelperProcess("keys");
        }
    });

    connect(ui->comboBoxLogin, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index > -1) {
                    generateResponse();
                }
            });

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, [this]() {
                tryResponse();
            });

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, [this]() {
                reject();
            });

    connect(ui->lineEdit, &QLineEdit::textChanged,
            this, [okButton](const QString &text) {
                okButton->setEnabled(!text.trimmed().isEmpty());
            });

    // Hook up textEdit's textChanged signal
    connect(ui->textEdit, &QTextEdit::textChanged,
            this, [this]() {
                ui->textEdit->selectAll();
                ui->lineEdit->setFocus();
            });

    checkHelperFiles();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::generateResponse()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    Settings settings;
    QString configFile = settings.configFilePath();
    QString configDir  = QFileInfo(configFile).absolutePath();

    QString wordListPath = QDir(configDir).filePath("wordlist.rcc");

    if (QFile::exists(wordListPath)) {
        if (QResource::registerResource(wordListPath)) {
            qInfo().noquote() << Q_FUNC_INFO << "Loaded wordlist.rcc file.";
        } else {
            qCritical().noquote() << Q_FUNC_INFO << "Failed to register resource:" << wordListPath;
        }
    } else
    {
        qCritical().noquote() << "Missing resource file:" << wordListPath;
    }

    QStringList words;
    {
        QFile file(settings.getWordListFile()); // e.g. :/wordlist/...
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                const QString line = in.readLine().trimmed();
                if (!line.isEmpty())
                    words << line;
            }
            file.close();
        }
    }

    // Unregister after loading
    if (!QResource::unregisterResource(wordListPath))
    {
        qCritical().noquote() << Q_FUNC_INFO << "Failed to unregister " << wordListPath;
    }

    QByteArray responseBytes;
    {
        QStringList chosen;

        if (words.size() >= 3) {
            for (int i = 0; i < 3; ++i) {
                const int r = QRandomGenerator::global()->bounded(words.size());
                QString word = words.at(r);
                if (!word.isEmpty())
                    word = word.left(1).toUpper() + word.mid(1).toLower();
                chosen << word;
            }
            const int randomWordIndex = QRandomGenerator::global()->bounded(chosen.size());
            const int randomDigit = QRandomGenerator::global()->bounded(10);
            chosen[randomWordIndex].append(QString::number(randomDigit));
        }

        if (chosen.isEmpty()) {
            QString fallback;
            static const QString chars = QStringLiteral("ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789");
            for (int i = 0; i < 16; ++i) {
                const int r = QRandomGenerator::global()->bounded(chars.size());
                fallback.append(chars.at(r));
                if ((i + 1) % 4 == 0 && i != 15)
                    fallback.append('-');
            }
            chosen << fallback;
        }

        responseBytes = chosen.join("-").toUtf8();

        for (QString &w : chosen) w.fill(QChar('\0'));
        chosen.clear();
        chosen.squeeze();
    }

    responseHash = QCryptographicHash::hash(responseBytes, QCryptographicHash::Sha512);

    const QString recipient = keys.value(ui->comboBoxLogin->currentText());
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QString text = QString::fromUtf8(process->readAllStandardOutput());
        ui->textEdit->setText(text);

        // Copy to clipboard right after setting the text
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    });

    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        qDebug() << "GPG error:" << QString::fromUtf8(process->readAllStandardError());
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus status) {
                qDebug() << "GPG finished with code" << exitCode << "status" << status;
            });

    process->start("gpg", {"-ea", "-r", recipient});

    QByteArray temp = "\nYour Response to login is:\n" + responseBytes + "\n\n";
    process->write(temp);
    process->closeWriteChannel();

    temp.fill('\0');
    temp.clear();

    responseBytes.fill('\0');
    responseBytes.clear();

    QByteArray hashHex = responseHash.toHex();
    qDebug().noquote() << Q_FUNC_INFO
                       << "Response hash:" << hashHex;
    std::fill(hashHex.begin(), hashHex.end(), '\0');
    hashHex.clear();

    QApplication::restoreOverrideCursor();
}

void LoginDialog::tryResponse()
{

        QByteArray enteredBytes = ui->lineEdit->text().toUtf8();
        QByteArray enteredHash = QCryptographicHash::hash(enteredBytes, QCryptographicHash::Sha512);
        std::fill(enteredBytes.begin(), enteredBytes.end(), '\0');
        enteredBytes.clear();

        if (enteredHash == responseHash) {
            accept();
        } else {
            if (errorCount >= 2) {
                reject();
            } else {
                ui->lineEdit->clear();
                ui->lineEdit->setFocus();
                errorCount++;
            }
        }

}

bool LoginDialog::hasKeys() const {
    return ui->comboBoxLogin->count() > 0;
}

void LoginDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!ui->textEdit->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(ui->textEdit->toPlainText(), QClipboard::Clipboard);
    }
}

void LoginDialog::checkHelperFiles()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString pwdhlpExe = appDir + "/pwdhlp.exe";
    QString pwdhlpNoExt = appDir + "/pwdhlp";
    QString pwdhlpRcc = appDir + "/pwdhlp.rcc";

    // Check existence of help system
    bool exeExists = QFile::exists(pwdhlpExe) || QFile::exists(pwdhlpNoExt);
    bool rccExists = QFile::exists(pwdhlpRcc);

    // If missing, disable Help button
    if (!(exeExists && rccExists)) {
        QPushButton *helpButton = ui->buttonBox->button(QDialogButtonBox::Help);
        if (helpButton) {
            helpButton->setEnabled(false);
        }
    }
}


