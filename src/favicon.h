#pragma once

#include <QObject>
#include <QIcon>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSize>

class Favicon : public QObject
{
    Q_OBJECT
public:
    explicit Favicon(const QString &input, QObject *parent = nullptr);

signals:
    void faviconReady(const QIcon &icon);

private slots:
    void onIcoFinished();
    void onHtmlFinished();
    void onIconFinished();
    void onTimeout();

private:
    struct IconCandidate {
        QUrl url;
        QSize size;
        QString type;
    };

    QList<QUrl> normalizeInput(const QString &input);
    void tryNextBaseUrl();
    void startForBaseUrl(const QUrl &base);

    void fetchHtml();
    void chooseAndFetchIcon(const QList<IconCandidate> &candidates);
    int scoreSize(const QSize &sz) const;

    QNetworkRequest makeRequest(const QUrl &url) const;
    QIcon iconFromData(const QByteArray &data,
                       const QString &typeHint,
                       const QUrl &urlHint) const;

    QIcon m_defaultIcon;
    QNetworkAccessManager m_manager;

    QList<QUrl> m_candidates;
    int m_candidateIndex = 0;

    QNetworkReply *m_replyIco = nullptr;
    QNetworkReply *m_replyHtml = nullptr;
    QNetworkReply *m_replyIcon = nullptr;

    QTimer m_timeout;
    QUrl m_baseUrl;
    bool m_emitted = false;
};
