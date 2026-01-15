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


#include "logindialog.h"
#include "ui_logindialog.h"

#include "dbutils.h"
#include "dbutils.h"
#include "settings.h"
#include "utils.h"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtGuiDepends>


LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Challenge"));
    ui->textEditChallenge->setReadOnly(true);
    ui->textEditChallenge->setLineWrapMode(QTextEdit::NoWrap);
    ui->textEditChallenge->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);
    okButton->setDefault(true);

    {
        auto list = DbUtils::fetchKeys(this);

        ui->comboBoxLogin->clear();
        keys.clear();

        for (int i = 0; i < list.size(); ++i) {
            const KeyEntry &entry = list.at(i);

            // Avoid showing duplicate text when label == key
            const QString displayText =
                (entry.label == entry.key)
                    ? entry.label
                    : entry.label + " (" + entry.key + ")";

            ui->comboBoxLogin->addItem(displayText, entry.key);
        }

        if (ui->comboBoxLogin->count() > 0) {
            ui->comboBoxLogin->setCurrentIndex(0);
            generateChallenge();
        }
    }

    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox->button(QDialogButtonBox::Help),
            &QPushButton::clicked,
            this,
            [this]() {

                checkOnlineHelp([this](bool reachable) {

                    if (reachable) {
                        QDesktopServices::openUrl(
                            QUrl(getHelpBaseUrl("challenge-response"))
                            );
                    } else if (localHelpAvailable()) {
                        launchHelperProcess("challenge-response");
                    } else {
                        QMessageBox::warning(
                            this,
                            tr("Help Error"),
                            tr(MSG_NO_HELP)
                            );
                    }
                });
            });

    connect(ui->comboBoxLogin, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index > -1) {
                    generateChallenge();
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

    connect(ui->lineEditChallengeResponse, &QLineEdit::textChanged,
            this, [okButton](const QString &text) {
                okButton->setEnabled(!text.trimmed().isEmpty());
            });

    // Hook up textEdit's textChanged signal
    connect(ui->textEditChallenge, &QTextEdit::textChanged,
            this, [this]() {
                ui->textEditChallenge->selectAll();
                ui->lineEditChallengeResponse->setFocus();
            });


    QIcon responseIcon(":/menus/glyphs/admin_panel_settings_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg");
    QAction *responseAction = ui->lineEditChallengeResponse->addAction(
        responseIcon,
        QLineEdit::LeadingPosition
        );
    Q_UNUSED(responseAction);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::generateChallenge()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->textEditChallenge->clear();
    ui->textEditChallenge->setPlaceholderText(tr("Loading challenge.."));

    Settings settings;
    QString configFile = settings.configFilePath();
    QString configDir  = QFileInfo(configFile).absolutePath();

    QString wordListPath = QDir(configDir).filePath("wordlist.rcc");

    if (QFile::exists(wordListPath)) {
        if (QResource::registerResource(wordListPath)) {
            qInfo().noquote() << "Loaded wordlist.rcc file.";
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

        if (words.size() >= settings.getGeneratedPasswordLength()) {
            for (int i = 0; i < settings.getGeneratedPasswordLength(); ++i) {
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

            static const QString chars =
                QStringLiteral("ABCDEFGHJKMNPQRSTUVWXYZ"
                               "abcdefghjkmnpqrstuvwxyz"
                               "23456789"
                               "@!$&");

            for (int i = 0; i < 28; ++i) {
                const int r = QRandomGenerator::global()->bounded(chars.size());
                fallback.append(chars.at(r));

                // Insert hyphens every 4 characters (except at the end)
                if ((i % 4) == 3 && i != 27)
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

    const QString recipient = ui->comboBoxLogin->currentData().toString();
    QProcess *process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QString text = QString::fromUtf8(process->readAllStandardOutput());
        ui->textEditChallenge->setText(text);
        ui->textEditChallenge->moveCursor(QTextCursor::Start);
        ui->textEditChallenge->verticalScrollBar()->setValue(0);

        // Copy to clipboard right after setting the text
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    });

    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        QByteArray error = process->readAllStandardError();
        qCritical().noquote() << Q_FUNC_INFO  << "GPG error:" << QString::fromUtf8(error);
        if (QString::fromUtf8(error).contains("waiting for lock", Qt::CaseInsensitive))
            return;
        QMessageBox::critical(this,QApplication::applicationName(),QString::fromUtf8(error));
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus status) {
                qDebug().noquote() << Q_FUNC_INFO << "GPG finished with code" << exitCode << "status" << status;
            });

    process->start("gpg", {"-ea", "-r", recipient});

    QByteArray temp;
    temp.reserve(responseBytes.size() + 64);
    temp.append(tr("\nYour Response is:\n").toUtf8());
    temp.append(responseBytes);
    temp.append("\n\n");

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
    QByteArray enteredBytes = ui->lineEditChallengeResponse->text().toUtf8();
    QByteArray enteredHash = QCryptographicHash::hash(enteredBytes, QCryptographicHash::Sha512);

    enteredBytes.fill('\0');
    if (enteredHash == responseHash) {
        accept();
        return;
    }

    errorCount++;
    if (errorCount >= 3) {
        reject();
        return;
    }

    ui->lineEditChallengeResponse->clear();
    ui->lineEditChallengeResponse->setFocus();
}


bool LoginDialog::hasKeys() const {
    return ui->comboBoxLogin->count() > 0;
}

void LoginDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!ui->textEditChallenge->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(ui->textEditChallenge->toPlainText(), QClipboard::Clipboard);
    }
}
