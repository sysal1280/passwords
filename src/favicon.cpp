#include "favicon.h"

#include <QPixmap>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QPainter>
#include <QDebug>

static const char kChromeWin11UA[] =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/122.0.0.0 Safari/537.36";

Favicon::Favicon(const QString &input, QObject *parent)
    : QObject(parent)
{
    m_defaultIcon = QIcon(":/menus/glyphs/password_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg");

    m_candidates = normalizeInput(input);

    if (m_candidates.isEmpty()) {
        m_emitted = true;

        // Emit asynchronously so UI is ready
        QMetaObject::invokeMethod(
            this,
            [this]() { emit faviconReady(m_defaultIcon); },
            Qt::QueuedConnection
            );
        return;
    }

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(8000);
    connect(&m_timeout, &QTimer::timeout, this, &Favicon::onTimeout);

    tryNextBaseUrl();
}

QList<QUrl> Favicon::normalizeInput(const QString &input)
{
    QString trimmed = input.trimmed();

    if (trimmed.isEmpty())
        return {};

    // Add https:// if missing
    if (!trimmed.startsWith("http://") && !trimmed.startsWith("https://"))
        trimmed = "https://" + trimmed;

    QUrl url(trimmed);
    if (!url.isValid())
        return {};

    QString host = url.host();
    if (host.isEmpty())
        return {};

    QStringList hosts;
    hosts << host;
    if (!host.startsWith("www."))
        hosts << "www." + host;

    QStringList schemes;
    schemes << "https" << "http";

    QList<QUrl> out;
    for (const QString &scheme : std::as_const(schemes)) {
        for (const QString &h : std::as_const(hosts)) {
            QUrl u;
            u.setScheme(scheme);
            u.setHost(h);
            u.setPath("/");
            out << u;
        }
    }

    return out;
}

void Favicon::tryNextBaseUrl()
{
    if (m_emitted)
        return;

    if (m_candidateIndex >= m_candidates.size()) {
        m_emitted = true;
        emit faviconReady(m_defaultIcon);
        return;
    }

    m_baseUrl = m_candidates[m_candidateIndex++];
    startForBaseUrl(m_baseUrl);
}

void Favicon::startForBaseUrl(const QUrl &base)
{
    // Try /favicon.ico
    QUrl icoUrl = base;
    icoUrl.setPath("/favicon.ico");

    m_replyIco = m_manager.get(makeRequest(icoUrl));
    connect(m_replyIco, &QNetworkReply::finished, this, &Favicon::onIcoFinished);

    m_timeout.start();
}

QNetworkRequest Favicon::makeRequest(const QUrl &url) const
{
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", kChromeWin11UA);
    req.setRawHeader("Accept",
                     "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");
    return req;
}

void Favicon::onIcoFinished()
{
    if (!m_replyIco)
        return;

    QNetworkReply *reply = m_replyIco;
    m_replyIco = nullptr;

    if (!m_emitted && reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QIcon icon = iconFromData(data, "image/x-icon", reply->url());
        if (!icon.isNull()) {
            m_emitted = true;
            m_timeout.stop();
            reply->deleteLater();
            emit faviconReady(icon);
            return;
        }
    }

    reply->deleteLater();

    if (!m_emitted)
        fetchHtml();
}

void Favicon::fetchHtml()
{
    m_replyHtml = m_manager.get(makeRequest(m_baseUrl));
    connect(m_replyHtml, &QNetworkReply::finished, this, &Favicon::onHtmlFinished);
}

int Favicon::scoreSize(const QSize &sz) const
{
    const int w = sz.width();
    const int h = sz.height();
    if (w <= 0 || h <= 0)
        return 0;

    if (w == 32 && h == 32) return 100;
    if (w == 16 && h == 16) return 90;
    if (w == 48 && h == 48) return 80;
    if (w == 192 && h == 192) return 70;

    int diff = qAbs(w - 32) + qAbs(h - 32);
    return 60 - qMin(diff, 60);
}

void Favicon::onHtmlFinished()
{
    if (!m_replyHtml)
        return;

    QByteArray data = m_replyHtml->readAll();
    m_replyHtml->deleteLater();
    m_replyHtml = nullptr;

    const QString html = QString::fromUtf8(data);

    QList<IconCandidate> candidates;

    QRegularExpression linkRe("<link[^>]*>",
                              QRegularExpression::CaseInsensitiveOption |
                                  QRegularExpression::DotMatchesEverythingOption);
    auto it = linkRe.globalMatch(html);

    while (it.hasNext()) {
        QString tag = it.next().captured(0);

        QRegularExpression relRe("rel\\s*=\\s*\"([^\"]*)\"|rel\\s*=\\s*'([^']*)'",
                                 QRegularExpression::CaseInsensitiveOption);
        auto relMatch = relRe.match(tag);
        if (!relMatch.hasMatch())
            continue;

        QString rel = relMatch.captured(1);
        if (rel.isEmpty())
            rel = relMatch.captured(2);
        rel = rel.toLower();

        if (!rel.contains("icon"))
            continue;

        QRegularExpression hrefRe("href\\s*=\\s*\"([^\"]*)\"|href\\s*=\\s*'([^']*)'",
                                  QRegularExpression::CaseInsensitiveOption);
        auto hrefMatch = hrefRe.match(tag);
        if (!hrefMatch.hasMatch())
            continue;

        QString href = hrefMatch.captured(1);
        if (href.isEmpty())
            href = hrefMatch.captured(2);

        QSize size;
        QRegularExpression sizesRe("sizes\\s*=\\s*\"([^\"]*)\"|sizes\\s*=\\s*'([^']*)'",
                                   QRegularExpression::CaseInsensitiveOption);
        auto sizesMatch = sizesRe.match(tag);
        if (sizesMatch.hasMatch()) {
            QString s = sizesMatch.captured(1);
            if (s.isEmpty())
                s = sizesMatch.captured(2);
            if (!s.isEmpty() && s != "any") {
                auto parts = s.split('x');
                if (parts.size() == 2)
                    size = QSize(parts[0].toInt(), parts[1].toInt());
            }
        }

        QString type;
        QRegularExpression typeRe("type\\s*=\\s*\"([^\"]*)\"|type\\s*=\\s*'([^']*)'",
                                  QRegularExpression::CaseInsensitiveOption);
        auto typeMatch = typeRe.match(tag);
        if (typeMatch.hasMatch()) {
            type = typeMatch.captured(1);
            if (type.isEmpty())
                type = typeMatch.captured(2);
            type = type.toLower();
        }

        QUrl iconUrl = m_baseUrl.resolved(QUrl(href));
        if (!iconUrl.isValid())
            continue;

        candidates.append({iconUrl, size, type});
    }

    if (candidates.isEmpty()) {
        tryNextBaseUrl();
        return;
    }

    chooseAndFetchIcon(candidates);
}

void Favicon::chooseAndFetchIcon(const QList<IconCandidate> &candidates)
{
    int bestScore = -1;
    IconCandidate best;

    for (const auto &c : candidates) {
        int s = scoreSize(c.size);
        if (s > bestScore) {
            bestScore = s;
            best = c;
        }
    }

    m_replyIcon = m_manager.get(makeRequest(best.url));
    connect(m_replyIcon, &QNetworkReply::finished, this, &Favicon::onIconFinished);
}

QIcon Favicon::iconFromData(const QByteArray &data,
                            const QString &typeHint,
                            const QUrl &urlHint) const
{
    QString type = typeHint.toLower();
    QString path = urlHint.path().toLower();

    bool isSvg = type.contains("svg") || path.endsWith(".svg");

    if (isSvg) {
        QSvgRenderer renderer(data);
        if (!renderer.isValid())
            return QIcon();

        QPixmap pix(32, 32);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        renderer.render(&p);
        return QIcon(pix);
    }

    QPixmap pix;
    if (!pix.loadFromData(data))
        return QIcon();
    return QIcon(pix);
}

void Favicon::onIconFinished()
{
    if (!m_replyIcon)
        return;

    QNetworkReply *reply = m_replyIcon;
    m_replyIcon = nullptr;

    QIcon result = m_defaultIcon;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        QIcon icon = iconFromData(data, type, reply->url());
        if (!icon.isNull())
            result = icon;
    }

    reply->deleteLater();

    if (!m_emitted) {
        m_emitted = true;
        m_timeout.stop();
        emit faviconReady(result);
    }
}

void Favicon::onTimeout()
{
    if (m_emitted)
        return;

    if (m_replyIco) {
        m_replyIco->abort();
        m_replyIco->deleteLater();
        m_replyIco = nullptr;
    }
    if (m_replyHtml) {
        m_replyHtml->abort();
        m_replyHtml->deleteLater();
        m_replyHtml = nullptr;
    }
    if (m_replyIcon) {
        m_replyIcon->abort();
        m_replyIcon->deleteLater();
        m_replyIcon = nullptr;
    }

    tryNextBaseUrl();
}
