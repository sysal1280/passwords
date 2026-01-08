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


#ifndef DBMAINTENANCE_H
#define DBMAINTENANCE_H

#include "scopedcursor.h"

#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QDialogButtonBox>

class DbMaintenance : public QDialog
{
    Q_OBJECT

public:
    explicit DbMaintenance(QSqlDatabase db, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_db(std::move(db))
        , m_statusLabel(nullptr)
        , m_progress(nullptr)
        , m_startBtn(nullptr)
        , m_closeBtn(nullptr)
    {
        setWindowTitle("Database Maintenance");
        setModal(true);
        setFixedSize(380, 180);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(8);

        m_statusLabel = new QLabel("Ready to begin maintenance.", this);
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("font-size: 13px;");
        layout->addWidget(m_statusLabel);

        m_progress = new QProgressBar(this);
        m_progress->setRange(0, 6);
        m_progress->setValue(0);
        m_progress->setFixedHeight(18);
        layout->addWidget(m_progress);
        layout->addSpacing(20);

        auto *buttonBox = new QDialogButtonBox(this);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        m_startBtn = buttonBox->addButton("Start", QDialogButtonBox::ActionRole);
        m_closeBtn = buttonBox->button(QDialogButtonBox::Close);

        layout->addWidget(buttonBox);

        connect(m_startBtn, &QPushButton::clicked,
                this, &DbMaintenance::startMaintenance);

        connect(m_closeBtn, &QPushButton::clicked,
                this, &QDialog::close);
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        if (!m_startBtn->isEnabled()) {
            QMessageBox::warning(this, "Maintenance",
                                 "Maintenance in progress. Please wait...");
            event->ignore();
            return;
        }
        QDialog::closeEvent(event);
    }

    void reject() override
    {
        if (!m_startBtn->isEnabled()) {
            QMessageBox::warning(this, "Maintenance",
                                 "Maintenance in progress. Please wait...");
            return;
        }

        QDialog::reject();
    }

private slots:
    void startMaintenance()
    {
        m_startBtn->setEnabled(false);
        m_statusLabel->setText("Starting maintenance...");

        // QThread::create returns a heap object; we tie its lifetime to this dialog.
        QThread *thread = QThread::create([this]() {
            runMaintenance();
        });

        thread->setObjectName("DbMaintenanceThread");

        connect(thread, &QThread::finished,
                this, &DbMaintenance::maintenanceFinished);
        connect(thread, &QThread::finished,
                thread, &QObject::deleteLater);

        thread->start();
    }

    void updateStatus(const QString &text, int progress)
    {
        m_statusLabel->setText(text);
        m_progress->setValue(progress);
    }

    void maintenanceFinished()
    {
        m_statusLabel->setText("Maintenance complete.");
        m_startBtn->setEnabled(true);
    }

private:
    void runMaintenance()
    {
        ScopedCursor wait(Qt::WaitCursor);

        auto post = [this](const QString &text, int progress) {
            QMetaObject::invokeMethod(this, "updateStatus",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, text),
                                      Q_ARG(int, progress));
        };

        auto pause = []() {
            QThread::msleep(450);
        };

        // Progress value is passed explicitly instead of reading widget state in worker thread
        auto fail = [this, post](const QString &step, const QString &error, int progress) {
            post(step + " failed: " + error, progress);

            QMetaObject::invokeMethod(this,
                                      [this, step, error]() {
                                          QMessageBox::critical(this, "Maintenance Error",
                                                                step + " failed:\n" + error);
                                          m_startBtn->setEnabled(true);
                                      },
                                      Qt::QueuedConnection);
        };

        QString err;

        post("Running integrity check...", 1);
        if (!runQuery("PRAGMA integrity_check;", err)) {
            fail("Integrity check", err, 1);
            return;
        }
        pause();

        post("Running quick check...", 2);
        if (!runQuery("PRAGMA quick_check;", err)) {
            fail("Quick check", err, 2);
            return;
        }
        pause();

        post("Analyzing database...", 3);
        if (!runQuery("ANALYZE;", err)) {
            fail("Analyze", err, 3);
            return;
        }
        pause();

        post("Reindexing...", 4);
        if (!runQuery("REINDEX;", err)) {
            fail("Reindex", err, 4);
            return;
        }
        pause();

        post("Checking WAL mode...", 5);
        const QString mode = getSingleValue("PRAGMA journal_mode;", err);
        if (mode.isEmpty() && !err.isEmpty()) {
            fail("Check WAL mode", err, 5);
            return;
        }

        if (mode.toLower() == "wal") {
            if (!runQuery("PRAGMA wal_checkpoint(FULL);", err)) {
                fail("WAL checkpoint", err, 5);
                return;
            }
        }
        pause();

        post("Vacuuming...", 6);
        if (!runQuery("VACUUM;", err)) {
            fail("Vacuum", err, 6);
            return;
        }
        pause();
    }

    bool runQuery(const QString &sql, QString &errorOut)
    {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            errorOut = q.lastError().text();
            return false;
        }
        return true;
    }

    QString getSingleValue(const QString &sql, QString &errorOut)
    {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            errorOut = q.lastError().text();
            return QString();
        }
        if (q.next())
            return q.value(0).toString();
        return QString();
    }

private:
    QSqlDatabase  m_db;
    QLabel       *m_statusLabel;
    QProgressBar *m_progress;
    QPushButton  *m_startBtn;
    QPushButton  *m_closeBtn;
};

#endif // DBMAINTENANCE_H
