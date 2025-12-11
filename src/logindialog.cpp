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
#include "settings.h"
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
                        const QString label = query.value(0).toString();
                        const QString key   = query.value(1).toString();
                        ui->comboBoxLogin->addItem(label);
                        keys.insert(label, key);
                    }

                    // --- activate first entry and update clipboard ---
                    if (ui->comboBoxLogin->count() > 0) {
                        ui->comboBoxLogin->setCurrentIndex(0);
                        on_comboBoxLogin_currentIndexChanged(0);
                    }
                }
                db.close();
            } else {
                QMessageBox::critical(this, QString(), db.lastError().text());
            }
        }
        // remove connection after db object is destroyed
        QSqlDatabase::removeDatabase(connectionName);

    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, this, [=]() {
        QMessageBox::information(this, tr("Help"),
                                 tr("This is where you show help text or open documentation."));
    });

    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &LoginDialog::onButtonBoxClicked);


}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_comboBoxLogin_currentIndexChanged(int index)
{
    if (index <= -1)
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString wordListRcc = QDir(configDir).filePath("wordlist.rc");

    if (!QResource::registerResource(wordListRcc)) {
        QMessageBox::warning(this,
                             tr("Word List Missing"),
                             tr("Could not load wordlist resource file.\n"
                                "Please select a wordlist from Preferences → Passwords tab."));
        QApplication::restoreOverrideCursor();
        return;
    }

    QStringList words;
    {
        QFile file(Settings::getWordListFile()); // e.g. :/wordlist/...
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
    QResource::unregisterResource(wordListRcc);

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
    qDebug() << "response hash:" << hashHex;
    std::fill(hashHex.begin(), hashHex.end(), '\0');
    hashHex.clear();

    QApplication::restoreOverrideCursor();
}

void LoginDialog::onButtonBoxClicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
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
    } else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::RejectRole) {
        reject();
    } else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::HelpRole) {
        // Do nothing here – helpRequested() will be emitted and handled
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

void LoginDialog::on_textEdit_textChanged()
{
    ui->textEdit->selectAll();
    ui->lineEdit->setFocus();
}

