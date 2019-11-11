#include "library/aoide/task/resolvetracksbyurltask.h"

#include <QJsonArray>

#include "util/encodedurl.h"
#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("aoide ResolveTracksByUrlTask");

network::JsonWebRequest resolveTracksByUrlTaskRequest(
        const QString& collectionUid,
        QList<QUrl> trackUrls) {
    QUrlQuery query;
    query.addQueryItem("collectionUid", collectionUid);
    QJsonArray encodedTrackUrls;
    for (const auto& trackUrl : trackUrls) {
        encodedTrackUrls.append(
                EncodedUrl::fromUrl(trackUrl).toString());
    }
    return network::JsonWebRequest{
            network::HttpRequestMethod::Post,
            QStringLiteral("/tracks/resolve"),
            query,
            QJsonDocument(encodedTrackUrls)};
}

} // anonymous namespace

namespace aoide {

ResolveTracksByUrlTask::ResolveTracksByUrlTask(
        QNetworkAccessManager* networkAccessManager,
        QUrl baseUrl,
        const QString& collectionUid,
        QList<QUrl> trackUrls,
        QObject* parent)
        : JsonWebTask(
                  networkAccessManager,
                  std::move(baseUrl),
                  resolveTracksByUrlTaskRequest(collectionUid, trackUrls),
                  parent),
          m_unresolvedTrackUrls(std::move(trackUrls)) {
}

void ResolveTracksByUrlTask::connectSlots() {
    JsonWebTask::connectSlots();
    connect(this,
            &network::JsonWebTask::finished,
            this,
            &ResolveTracksByUrlTask::slotFinished);
}

void ResolveTracksByUrlTask::slotFinished(
        network::JsonWebResponse response) {
    if (!response.isStatusCodeSuccess()) {
        emit failed(std::move(response));
        return;
    }
    DEBUG_ASSERT(response.statusCode == network::kHttpStatusCodeOk);

    VERIFY_OR_DEBUG_ASSERT(response.content.isArray()) {
        kLogger.warning()
                << "Invalid JSON content"
                << response.content;
        emit failed(std::move(response));
        return;
    }
    const auto jsonArray = response.content.array();

    QMap<QUrl, QString> resolvedTrackUrls;
    for (const auto& elem : jsonArray) {
        VERIFY_OR_DEBUG_ASSERT(elem.isArray()) {
            kLogger.warning()
                    << "Invalid JSON content"
                    << response.content;
            emit failed(std::move(response));
            return;
        }
        auto pair = elem.toArray();
        VERIFY_OR_DEBUG_ASSERT(pair.size() == 2) {
            kLogger.warning()
                    << "Invalid JSON content"
                    << response.content;
            emit failed(std::move(response));
            return;
        }
        auto url = EncodedUrl::fromUrlEncoded(
                pair[0].toString().toUtf8())
                           .toUrl();
        VERIFY_OR_DEBUG_ASSERT(url.isValid()) {
            kLogger.warning()
                    << "Invalid encoded URL"
                    << pair[0].toString();
            emit failed(std::move(response));
            return;
        }
        auto uid = pair[1].toString();
        VERIFY_OR_DEBUG_ASSERT(!uid.isEmpty()) {
            kLogger.warning()
                    << "Missing track UID";
            emit failed(std::move(response));
            return;
        }
        DEBUG_ASSERT(!resolvedTrackUrls.contains(url));
        resolvedTrackUrls.insert(url, uid);
    }

    QList<QUrl> unresolvedTrackUrls;
    DEBUG_ASSERT(resolvedTrackUrls.size() <= m_unresolvedTrackUrls.size());
    unresolvedTrackUrls.reserve(
            m_unresolvedTrackUrls.size() -
            resolvedTrackUrls.size());
    for (const auto& url : m_unresolvedTrackUrls) {
        if (!resolvedTrackUrls.contains(url)) {
            unresolvedTrackUrls.append(url);
        }
    }

    emit finished(
            resolvedTrackUrls,
            unresolvedTrackUrls);
}

} // namespace aoide

} // namespace mixxx
