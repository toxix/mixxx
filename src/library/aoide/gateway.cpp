#include "library/aoide/gateway.h"

#include <QMetaObject>
#include <QRegularExpression>
#include <QThread>
#include <mutex> // std::once_flag

#include "library/aoide/tag/hashtagcommentstagger.h"
#include "library/aoide/task/resolvetracksbyurltask.h"
#include "library/aoide/task/searchtrackstask.h"
#include "library/aoide/trackexporter.h"
#include "network/jsonwebclient.h"
#include "track/trackfile.h"
#include "util/encodedurl.h"
#include "util/logger.h"

// TODO:
// - Replace QMetaObject::invokeMethod() with type-safe
//   function pointer variant (since Qt 5.6)

namespace mixxx {

namespace {

std::once_flag registerMetaTypesOnceFlag;

void registerMetaTypesOnce() {
    qRegisterMetaType<AoidePagination>();

    qRegisterMetaType<AoideEntityRevision>();
    qRegisterMetaType<AoideEntityHeader>();

    qRegisterMetaType<AoideCollection>();
    qRegisterMetaType<AoideCollectionEntity>();
    qRegisterMetaType<QVector<AoideCollectionEntity>>();

    qRegisterMetaType<AoideTitle>();
    qRegisterMetaType<AoideTitleVector>();
    qRegisterMetaType<AoideActor>();
    qRegisterMetaType<AoideActorVector>();
    qRegisterMetaType<AoideTag>();
    qRegisterMetaType<AoideTagVector>();

    qRegisterMetaType<AoideTrackOrAlbum>();
    qRegisterMetaType<AoideRelease>();
    qRegisterMetaType<AoideAlbum>();
    qRegisterMetaType<AoideTrack>();
    qRegisterMetaType<AoideTrackEntity>();
    qRegisterMetaType<QVector<AoideTrackEntity>>();

    qRegisterMetaType<AoideTagFacetCount>();
    qRegisterMetaType<QVector<AoideTagFacetCount>>();
    qRegisterMetaType<AoideTagCount>();
    qRegisterMetaType<QVector<AoideTagCount>>();

    qRegisterMetaType<AoidePlaylist>();
    qRegisterMetaType<AoidePlaylistEntry>();
    qRegisterMetaType<AoidePlaylistEntity>();
    qRegisterMetaType<QVector<AoidePlaylistEntity>>();
    qRegisterMetaType<AoidePlaylistBrief>();
    qRegisterMetaType<AoidePlaylistBriefEntries>();
    qRegisterMetaType<AoidePlaylistBriefEntity>();
    qRegisterMetaType<QVector<AoidePlaylistBriefEntity>>();
}

const Logger kLogger("aoide Gateway");

const QString kReplaceMode = QStringLiteral("update-or-create");

} // anonymous namespace

namespace aoide {

SearchTracksTask* Gateway::searchTracks(
        const QString& collectionUid,
        const QJsonObject& baseQuery,
        const QStringList& searchTerms,
        const AoidePagination& pagination,
        QObject* parent) {
    auto task = new SearchTracksTask(
            m_networkAccessManager,
            m_baseUrl,
            collectionUid,
            baseQuery,
            searchTerms,
            pagination,
            parent);
    task->moveToThread(thread());
    return task;
}

ResolveTracksByUrlTask* Gateway::resolveTracksByUrl(
        const QString& collectionUid,
        QList<QUrl> trackUrls,
        QObject* parent) {
    auto task = new ResolveTracksByUrlTask(
            m_networkAccessManager,
            m_baseUrl,
            collectionUid,
            std::move(trackUrls),
            parent);
    task->moveToThread(thread());
    return task;
}

Gateway::Gateway(
        QUrl baseUrl,
        Settings settings,
        QNetworkAccessManager* networkAccessManager,
        QObject* parent)
        : QObject(parent),
          m_baseUrl(std::move(baseUrl)),
          m_settings(std::move(settings)),
          m_networkAccessManager(networkAccessManager),
          m_hashtagCommentsTagger(std::make_unique<HashtagCommentsTagger>()),
          m_jsonWebClient(make_parented<network::JsonWebClient>(networkAccessManager, this)) {
    std::call_once(registerMetaTypesOnceFlag, registerMetaTypesOnce);
    DEBUG_ASSERT(m_baseUrl.isValid());
    DEBUG_ASSERT(m_networkAccessManager);
}

Gateway::~Gateway() {
}

AoideTrack Gateway::exportTrack(
        QString collectionUid,
        const Track& track) const {
    DEBUG_ASSERT(m_hashtagCommentsTagger);
    return TrackExporter(
            std::move(collectionUid), m_settings)
            .exportTrack(
                    track, *m_hashtagCommentsTagger);
}

void Gateway::connectSlots() {
    connect(m_jsonWebClient,
            &network::JsonWebClient::networkRequestFailed,
            this,
            &Gateway::onNetworkRequestFailed);
}

QUrl Gateway::resourceUrl(const QString& resourcePath) const {
    QUrl url = m_baseUrl;
    url.setPath(resourcePath);
    return url;
}

Gateway::RequestId Gateway::invokeShutdown() {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotShutdown",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId));
    return requestId;
}

void Gateway::slotShutdown(
        network::RequestId requestId) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    if (m_shutdownRequestId.isValid() && (m_shutdownRequestId != requestId)) {
        kLogger.warning()
                << "Shutdown has already been requested";
        return;
    }
    if (m_pendingWriteRequestId.isValid() || !m_queuedWriteRequests.isEmpty()) {
        kLogger.info()
                << "Delaying shutdown until all write requests have been finished";
        m_shutdownRequestId = requestId;
        return;
    }
    if (m_shutdownRequestId.isValid()) {
        kLogger.info()
                << "Shutting down after all write requests have been finished";
        m_shutdownRequestId.reset();
    }

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(requestId);
    if (!networkAccessManager) {
        return;
    }

    QUrl url = resourceUrl("/shutdown");

    QNetworkRequest request(url);

    QNetworkReply* reply = networkAccessManager->post(request, QByteArray());
    m_jsonWebClient->afterRequestSent(requestId, reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            &Gateway::onShutdownNetworkReplyFinished);
}

void Gateway::onShutdownNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Shutting down failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeAccepted);
}

Gateway::RequestId Gateway::invokeListCollections(AoidePagination pagination) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotListCollections",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(AoidePagination, std::move(pagination)));
    return requestId;
}

void Gateway::slotListCollections(
        network::RequestId requestId,
        AoidePagination pagination) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(requestId);
    if (!networkAccessManager) {
        return;
    }

    QUrl url = resourceUrl("/collections");
    QUrlQuery query;
    pagination.addToQuery(&query);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkAccessManager->get(request);
    m_jsonWebClient->afterRequestSent(requestId, reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            &Gateway::onListCollectionsNetworkReplyFinished);
}

void Gateway::onListCollectionsNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Listing collections failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    QVector<AoideCollectionEntity> result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        const auto jsonArray = jsonContent.array();
        result.reserve(jsonArray.size());
        for (const auto& jsonValue : jsonArray) {
            DEBUG_ASSERT(jsonValue.isArray());
            result.append(AoideCollectionEntity(jsonValue.toArray()));
        }
    }

    emit listCollectionsResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeCreateCollection(AoideCollection collection) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotCreateCollection",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(AoideCollection, std::move(collection)));
    return requestId;
}

void Gateway::slotCreateCollection(
        network::RequestId requestId,
        AoideCollection collection) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Post);
    writeRequest.id = requestId;
    writeRequest.path = "/collections";
    writeRequest.jsonContent = QJsonDocument(collection).toJson(QJsonDocument::Compact);
    writeRequest.finishedHandler = &Gateway::onCreateCollectionNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onCreateCollectionNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Creating a new collection failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeCreated);

    AoideEntityHeader result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        result = AoideEntityHeader(jsonContent.array());
    }

    emit createCollectionResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeUpdateCollection(AoideCollectionEntity collectionEntity) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotUpdateCollection",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(AoideCollectionEntity, std::move(collectionEntity)));
    return requestId;
}

void Gateway::slotUpdateCollection(
        network::RequestId requestId, AoideCollectionEntity collectionEntity) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Put);
    writeRequest.id = requestId;
    writeRequest.path = "/collections/" + collectionEntity.header().uid();
    writeRequest.jsonContent = QJsonDocument(collectionEntity).toJson(QJsonDocument::Compact);
    writeRequest.finishedHandler = &Gateway::onUpdateCollectionNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onUpdateCollectionNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Updating a collection failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    AoideEntityHeader result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        result = AoideEntityHeader(jsonContent.array());
    }

    emit updateCollectionResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeDeleteCollection(QString collectionUid) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotDeleteCollection",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)));
    return requestId;
}

void Gateway::slotDeleteCollection(
        network::RequestId requestId,
        QString collectionUid) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Delete);
    writeRequest.id = requestId;
    writeRequest.path = "/collections/" + collectionUid;
    writeRequest.finishedHandler = &Gateway::onDeleteCollectionNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onDeleteCollectionNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Delting a collection failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeNoContent);

    emit deleteCollectionResult(requestId);
}

Gateway::RequestId Gateway::invokeReplaceTracks(
        QString collectionUid,
        QList<AoideTrack> tracks) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotReplaceTracks",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QList<AoideTrack>, std::move(tracks)));
    return requestId;
}

void Gateway::slotReplaceTracks(
        network::RequestId requestId,
        QString collectionUid,
        QList<AoideTrack> tracks) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    QJsonArray jsonReplacements;
    for (auto track : qAsConst(tracks)) {
        jsonReplacements += QJsonObject{
                {"mediaUri", track.mediaSource().uri().toString()},
                {"track", track.intoJsonValue()},
        };
    }
    QByteArray jsonContent = QJsonDocument(QJsonObject{
                                                   {"mode", kReplaceMode},
                                                   {"replacements", jsonReplacements},
                                           })
                                     .toJson(QJsonDocument::Compact);
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "QJsonDocument" << jsonContent.toStdString().c_str();
    }

    WriteRequest writeRequest(WriteRequestMethod::Post);
    writeRequest.id = requestId;
    writeRequest.path = "/tracks/replace";
    if (!collectionUid.isEmpty()) {
        writeRequest.query.addQueryItem("collectionUid", collectionUid);
    }
    writeRequest.jsonContent = std::move(jsonContent);
    writeRequest.finishedHandler = &Gateway::onReplaceTracksNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onReplaceTracksNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Replacing tracks failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    QJsonObject result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isObject()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        result = jsonContent.object();
    }

    emit replaceTracksResult(requestId, result);
}

Gateway::RequestId Gateway::invokeRelocateTracks(
        QString collectionUid,
        QList<QPair<QString, QString>> const& relocatedLocations) {
    QJsonArray relocateTracksParams;
    for (const auto& relocatedLocation : relocatedLocations) {
        const auto oldTrackFile = TrackFile(QDir(relocatedLocation.first));
        const auto oldSourceUri = EncodedUrl::fromUrl(oldTrackFile.toUrl());
        const auto newTrackFile = TrackFile(QDir(relocatedLocation.second));
        const auto newSourceUri = EncodedUrl::fromUrl(newTrackFile.toUrl());
        kLogger.debug()
                << "Relocating track file:"
                << oldSourceUri
                << "->"
                << newSourceUri;
        relocateTracksParams +=
                QJsonObject{
                        {
                                "predicate",
                                QJsonObject{{"exact", oldSourceUri.toString()}},
                        },
                        {
                                "replacement",
                                newSourceUri.toString(),
                        }};
    }
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(this, "relocateTracks",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QJsonArray, std::move(relocateTracksParams)));
    return requestId;
}

Gateway::RequestId Gateway::invokeRelocateAllTracks(
        QString collectionUid,
        const QDir& oldDir,
        const QDir& newDir) {
    const auto oldTrackDir = TrackFile(QDir(oldDir));
    const auto oldUriPrefix = EncodedUrl::fromUrl(oldTrackDir.toUrl());
    const auto newTrackDir = TrackFile(QDir(newDir));
    const auto newUriPrefix = EncodedUrl::fromUrl(newTrackDir.toUrl());
    kLogger.debug()
            << "Relocating all tracks in directory:"
            << oldUriPrefix
            << "->"
            << newUriPrefix;
    QJsonArray relocateTracksParams{
            QJsonObject{
                    {"predicate",
                            QJsonObject{{"prefix",
                                    QJsonValue(oldUriPrefix.toString() % '/')}}},
                    {"replacement",
                            QJsonValue(newUriPrefix.toString() % '/')}}};
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(this, "slotRelocateTracks",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QJsonArray, std::move(relocateTracksParams)));
    return requestId;
}

void Gateway::slotRelocateTracks(
        network::RequestId requestId,
        QString collectionUid,
        QJsonArray body) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Post);
    writeRequest.id = requestId;
    writeRequest.path = "/tracks/relocate";
    if (!collectionUid.isEmpty()) {
        writeRequest.query.addQueryItem("collectionUid", collectionUid);
    }
    writeRequest.jsonContent = QJsonDocument(std::move(body)).toJson(QJsonDocument::Compact);
    writeRequest.finishedHandler = &Gateway::onRelocateTracksNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onRelocateTracksNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Relocating tracks failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeNoContent);

    emit relocateTracksResult(requestId);
}

Gateway::RequestId Gateway::invokePurgeTracks(
        QString collectionUid,
        const QStringList& trackLocations) {
    QJsonArray purgeTracksParams;
    for (const auto& trackLocation : trackLocations) {
        const auto trackFile = TrackFile(QFileInfo(trackLocation));
        const auto trackUri = EncodedUrl::fromUrl(trackFile.toUrl());
        purgeTracksParams +=
                QJsonObject{
                        {
                                "exact",
                                trackUri.toString(),
                        }};
    }
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(this, "slotPurgeTracks",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QJsonArray, std::move(purgeTracksParams)));
    return requestId;
}

Gateway::RequestId Gateway::invokePurgeAllTracks(
        QString collectionUid,
        const QDir& rootDir) {
    const auto rootUri = EncodedUrl::fromUrl(TrackFile(rootDir).toUrl());
    auto purgeTracksParams = QJsonArray{
            QJsonObject{
                    {"prefix", QJsonValue(rootUri.toString() % '/')}}};
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(this, "slotPurgeTracks",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QJsonArray, std::move(purgeTracksParams)));
    return requestId;
}

void Gateway::slotPurgeTracks(
        network::RequestId requestId,
        QString collectionUid,
        QJsonArray body) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Post);
    writeRequest.id = requestId;
    writeRequest.path = "/tracks/purge";
    if (!collectionUid.isEmpty()) {
        writeRequest.query.addQueryItem("collectionUid", collectionUid);
    }
    writeRequest.jsonContent = QJsonDocument(std::move(body)).toJson(QJsonDocument::Compact);
    writeRequest.finishedHandler = &Gateway::onPurgeTracksNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onPurgeTracksNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Purging tracks failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeNoContent);

    emit purgeTracksResult(requestId);
}

void Gateway::enqueueWriteRequest(
        WriteRequest writeRequest) {
    m_queuedWriteRequests.enqueue(writeRequest);
    // Dequeue next write request if none is currently pending
    finishWriteRequest();
}

void Gateway::finishWriteRequest(
        RequestId requestId) {
    if (requestId.isValid()) {
        VERIFY_OR_DEBUG_ASSERT(
                !m_pendingWriteRequestId.isValid() ||
                requestId == m_pendingWriteRequestId) {
            kLogger.warning()
                    << "Finished write request"
                    << requestId
                    << "differs from pending write request"
                    << m_pendingWriteRequestId;
            return;
        }
    } else {
        if (m_pendingWriteRequestId.isValid()) {
            // This is expected behavior. The function is called
            // immediately after enqueing a new write request to
            // keep the processing of queued write requests going.
            kLogger.debug()
                    << "Cannot dequeue next write request due to pending write request"
                    << m_pendingWriteRequestId;
            return;
        }
    }
    m_pendingWriteRequestId.reset();
    DEBUG_ASSERT(!m_pendingWriteRequestId.isValid());
    if (m_queuedWriteRequests.isEmpty()) {
        kLogger.debug()
                << "No queued write requests";
        if (m_shutdownRequestId.isValid()) {
            // Continue delayed shutdown
            slotShutdown(m_shutdownRequestId);
        }
        return;
    }

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(m_queuedWriteRequests.head().id);
    VERIFY_OR_DEBUG_ASSERT(networkAccessManager) {
        kLogger.warning()
                << "No network access for sending write request"
                << m_queuedWriteRequests.head().id;
        return;
    }

    auto writeRequest = m_queuedWriteRequests.dequeue();
    kLogger.debug()
            << "Dequeued next write request"
            << writeRequest.id;
    if (m_queuedWriteRequests.size() > 0) {
        // The queue should mostly be empty after dequeuing the head
        // entry, i.e. the following log message should not appear
        // very often!
        kLogger.debug()
                << m_queuedWriteRequests.size()
                << "queued write request(s) remaining";
    }

    QUrl url = resourceUrl(writeRequest.path);
    if (!writeRequest.query.isEmpty()) {
        url.setQuery(writeRequest.query);
    }

    QNetworkReply* reply = nullptr;
    switch (writeRequest.method) {
    case WriteRequestMethod::Put: {
        reply = networkAccessManager->put(
                m_jsonWebClient->newRequest(url),
                writeRequest.jsonContent);
        break;
    }
    case WriteRequestMethod::Post:
        reply = networkAccessManager->post(
                m_jsonWebClient->newRequest(url),
                writeRequest.jsonContent);
        break;
    case WriteRequestMethod::Delete:
        DEBUG_ASSERT(writeRequest.jsonContent.isEmpty());
        reply = networkAccessManager->deleteResource(QNetworkRequest(url));
        break;
    }
    connect(reply,
            &QNetworkReply::finished,
            this,
            writeRequest.finishedHandler);
    DEBUG_ASSERT(!m_pendingWriteRequestId.isValid());
    m_pendingWriteRequestId = writeRequest.id;
    DEBUG_ASSERT(m_pendingWriteRequestId.isValid());
    m_jsonWebClient->afterRequestSent(writeRequest.id, reply);
}

void Gateway::onNetworkRequestFailed(
        RequestId requestId,
        QString errorMessage) {
    DEBUG_ASSERT(requestId.isValid());
    kLogger.warning()
            << "Network request"
            << requestId
            << "failed:"
            << errorMessage;
    if (requestId == m_pendingWriteRequestId) {
        // Clear the pending write request and continue processing
        // any queued write requests
        finishWriteRequest(requestId);
    }
    // Forward the signal
    emit networkRequestFailed(requestId, errorMessage);
}

Gateway::RequestId Gateway::invokeListTagsFacets(
        QString collectionUid,
        QSharedPointer<QStringList> facets,
        AoidePagination pagination) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotListTagsFacets",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QSharedPointer<QStringList>, std::move(facets)),
            Q_ARG(AoidePagination, std::move(pagination)));
    return requestId;
}

void Gateway::slotListTagsFacets(
        network::RequestId requestId,
        QString collectionUid,
        QSharedPointer<QStringList> facets,
        AoidePagination pagination) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(requestId);
    if (!networkAccessManager) {
        return;
    }

    QUrl url = resourceUrl("/tags/facets");
    QUrlQuery query;
    if (!collectionUid.isEmpty()) {
        query.addQueryItem("collectionUid", collectionUid);
    }
    if (facets) {
        query.addQueryItem("facet", facets->join(','));
    }
    pagination.addToQuery(&query);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkAccessManager->get(request);
    m_jsonWebClient->afterRequestSent(requestId, reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            &Gateway::onListTagsFacetsNetworkReplyFinished);
}

void Gateway::onListTagsFacetsNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Listing facets of tags failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    QVector<AoideTagFacetCount> result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        const auto jsonArray = jsonContent.array();
        result.reserve(jsonArray.size());
        for (const auto& jsonValue : jsonArray) {
            DEBUG_ASSERT(jsonValue.isObject());
            result.append(AoideTagFacetCount(jsonValue.toObject()));
        }
    }

    emit listTagsFacetsResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeListTags(
        QString collectionUid,
        QSharedPointer<QStringList> facets,
        AoidePagination pagination) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this, "slotListTags",
            Qt::QueuedConnection, // async,
            Q_ARG(mixxx::network::RequestId, requestId),
            Q_ARG(QString, std::move(collectionUid)),
            Q_ARG(QSharedPointer<QStringList>, std::move(facets)),
            Q_ARG(AoidePagination, std::move(pagination)));
    return requestId;
}

void Gateway::slotListTags(
        network::RequestId requestId,
        QString collectionUid,
        QSharedPointer<QStringList> facets,
        AoidePagination pagination) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(requestId);
    if (!networkAccessManager) {
        return;
    }

    QUrl url = resourceUrl("/tags");
    QUrlQuery query;
    if (!collectionUid.isEmpty()) {
        query.addQueryItem("collectionUid", collectionUid);
    }
    if (facets) {
        query.addQueryItem("facets", facets->join(','));
    }
    pagination.addToQuery(&query);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkAccessManager->get(request);
    m_jsonWebClient->afterRequestSent(requestId, reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            &Gateway::onListTagsNetworkReplyFinished);
}

void Gateway::onListTagsNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Listing tags failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    QVector<AoideTagCount> result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        const auto jsonArray = jsonContent.array();
        result.reserve(jsonArray.size());
        for (const auto& jsonValue : jsonArray) {
            DEBUG_ASSERT(jsonValue.isObject());
            result.append(AoideTagCount(jsonValue.toObject()));
        }
    }

    emit listTagsResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeCreatePlaylist(
        AoidePlaylist playlist) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            "slotCreatePlaylist",
#else
            [this, requestId, playlist] {
                this->slotCreatePlaylist(
                        requestId,
                        playlist);
            }
#endif
            ,
            Qt::QueuedConnection // async,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            ,
            Q_ARG(mixxx::network::RequestId, requestId),
            ,
            Q_ARG(AoidePlaylist, std::move(playlist))
#endif
    );
    return requestId;
}

void Gateway::slotCreatePlaylist(
        network::RequestId requestId,
        AoidePlaylist playlist) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Post);
    writeRequest.id = requestId;
    writeRequest.path = "/playlists";
    writeRequest.jsonContent = QJsonDocument(playlist).toJson(QJsonDocument::Compact);
    writeRequest.finishedHandler = &Gateway::onCreatePlaylistNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onCreatePlaylistNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Creating a new playlist failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeCreated);

    AoidePlaylistBriefEntity result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        result = AoidePlaylistBriefEntity(jsonContent.array());
    }

    emit createPlaylistResult(requestId, std::move(result));
}

Gateway::RequestId Gateway::invokeDeletePlaylist(
        QString playlistUid) {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            "slotDeletePlaylist",
#else
            [this, requestId, playlistUid] {
                this->slotDeletePlaylist(
                        requestId,
                        playlistUid);
            }
#endif
            ,
            Qt::QueuedConnection // async,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            ,
            Q_ARG(mixxx::network::RequestId, requestId),
            ,
            Q_ARG(QString, std::move(playlistUid))
#endif
    );
    return requestId;
}

void Gateway::slotDeletePlaylist(
        network::RequestId requestId,
        QString playlistUid) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    WriteRequest writeRequest(WriteRequestMethod::Delete);
    writeRequest.id = requestId;
    writeRequest.path = "/playlists/" + playlistUid;
    writeRequest.finishedHandler = &Gateway::onDeletePlaylistNetworkReplyFinished;

    enqueueWriteRequest(std::move(writeRequest));
}

void Gateway::onDeletePlaylistNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }
    finishWriteRequest(requestId);

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Deleting a playlist failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeNoContent);

    emit deletePlaylistResult(requestId);
}

Gateway::RequestId Gateway::invokeLoadPlaylistBriefs() {
    auto requestId = RequestId::nextValid();
    QMetaObject::invokeMethod(
            this,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            "slotLoadPlaylistBriefs",
#else
            [this, requestId] {
                this->slotLoadPlaylistBriefs(
                        requestId);
            }
#endif
            ,
            Qt::QueuedConnection // async,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            ,
            Q_ARG(mixxx::network::RequestId, requestId))
#endif
    );
    return requestId;
}

void Gateway::slotLoadPlaylistBriefs(
        RequestId requestId) {
    DEBUG_ASSERT(thread() == QThread::currentThread());

    QNetworkAccessManager* networkAccessManager =
            m_jsonWebClient->accessNetwork(requestId);
    if (!networkAccessManager) {
        return;
    }

    QUrl url = resourceUrl("/playlists");
    QUrlQuery query;
    // TODO: Add query parameters
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkAccessManager->get(request);
    m_jsonWebClient->afterRequestSent(requestId, reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            &Gateway::onLoadPlaylistBriefsNetworkReplyFinished);
}

void Gateway::onLoadPlaylistBriefsNetworkReplyFinished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());
    QJsonDocument jsonContent;
    const std::pair<RequestId, int> response =
            m_jsonWebClient->receiveReply(reply, &jsonContent);

    const RequestId requestId = response.first;
    VERIFY_OR_DEBUG_ASSERT(requestId.isValid()) {
        return;
    }

    const int statusCode = response.second;
    if (!network::JsonWebClient::isStatusCodeSuccess(statusCode)) {
        kLogger.warning()
                << "Loading playlist briefs failed: Network request"
                << requestId
                << "finished with status code"
                << statusCode;
        return;
    }
    DEBUG_ASSERT(statusCode == network::kHttpStatusCodeOk);

    QVector<AoidePlaylistBriefEntity> result;
    VERIFY_OR_DEBUG_ASSERT(jsonContent.isArray()) {
        kLogger.warning() << "Invalid JSON content" << jsonContent;
    }
    else {
        const auto jsonArray = jsonContent.array();
        result.reserve(jsonArray.size());
        for (const auto& jsonValue : jsonArray) {
            DEBUG_ASSERT(jsonValue.isArray());
            result.append(AoidePlaylistBriefEntity(jsonValue.toArray()));
        }
    }

    emit loadPlaylistBriefsResult(requestId, std::move(result));
}

} // namespace aoide

} // namespace mixxx
