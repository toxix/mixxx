#pragma once

#include <QDir>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QSharedPointer>
#include <QStringList>
#include <memory>

#include "library/aoide/domain/collection.h"
#include "library/aoide/domain/playlist.h"
#include "library/aoide/domain/track.h"
#include "library/aoide/settings.h"
#include "library/aoide/util.h"
#include "network/requestid.h"
#include "util/parented_ptr.h"

namespace mixxx {

namespace network {

class JsonWebClient;

}

namespace aoide {

class HashtagCommentsTagger;
class ResolveTracksByUrlTask;
class SearchTracksTask;

class Gateway : public QObject {
    Q_OBJECT

  public:
    Gateway(
            QUrl baseUrl,
            Settings settings,
            QNetworkAccessManager* networkAccessManager,
            QObject* parent = nullptr);
    ~Gateway() override;

    // Connect slots after moveToThread()
    void connectSlots();

    const Settings& settings() const {
        return m_settings;
    }

    AoideTrack exportTrack(
            QString collectionUid,
            const Track& track) const;

    SearchTracksTask* searchTracks(
            const QString& collectionUid,
            const QJsonObject& baseQuery,
            const QStringList& searchTerms,
            const AoidePagination& pagination,
            QObject* parent = nullptr);
    ResolveTracksByUrlTask* resolveTracksByUrl(
            const QString& collectionUid,
            QList<QUrl> trackUrls,
            QObject* parent = nullptr);

    typedef network::RequestId RequestId;

    RequestId invokeShutdown();

    RequestId invokeListCollections(
            AoidePagination pagination = AoidePagination());
    RequestId invokeCreateCollection(
            AoideCollection collection);
    RequestId invokeUpdateCollection(
            AoideCollectionEntity collectionEntity);
    RequestId invokeDeleteCollection(
            QString collectionUid);

    RequestId invokeReplaceTracks(
            QString collectionUid,
            QList<AoideTrack> tracks);
    RequestId invokeRelocateTracks(
            QString collectionUid,
            QList<QPair<QString, QString>> const& relocatedLocations);
    RequestId invokeRelocateAllTracks(
            QString collectionUid,
            const QDir& oldDir,
            const QDir& newDir);
    RequestId invokePurgeTracks(
            QString collectionUid,
            const QStringList& trackLocations);
    RequestId invokePurgeAllTracks(
            QString collectionUid,
            const QDir& rootDir);

    RequestId invokeListTagsFacets(
            QString collectionUid,
            QSharedPointer<QStringList> facets = QSharedPointer<QStringList>(),
            AoidePagination pagination = AoidePagination());
    RequestId invokeListTags(
            QString collectionUid,
            QSharedPointer<QStringList> facets = QSharedPointer<QStringList>(),
            AoidePagination pagination = AoidePagination());

    RequestId invokeCreatePlaylist(
            AoidePlaylist playlist);
    RequestId invokeDeletePlaylist(
            QString playlistUid);
    RequestId invokeLoadPlaylistBriefs();

  public slots:
    void slotShutdown(
            mixxx::network::RequestId requestId);

    void slotListCollections(
            mixxx::network::RequestId requestId,
            AoidePagination pagination);

    void slotCreateCollection(
            mixxx::network::RequestId requestId,
            AoideCollection collection);

    void slotUpdateCollection(
            mixxx::network::RequestId requestId,
            AoideCollectionEntity collectionEntity);

    void slotDeleteCollection(
            mixxx::network::RequestId requestId,
            QString collectionUid);

    void slotReplaceTracks(
            mixxx::network::RequestId requestId,
            QString collectionUid,
            QList<AoideTrack> tracks);
    void slotRelocateTracks(
            mixxx::network::RequestId requestId,
            QString collectionUid,
            QJsonArray body);
    void slotPurgeTracks(
            mixxx::network::RequestId requestId,
            QString collectionUid,
            QJsonArray body);

    void slotListTagsFacets(
            mixxx::network::RequestId requestId,
            QString collectionUid,
            QSharedPointer<QStringList> facets,
            AoidePagination pagination);

    void slotListTags(
            mixxx::network::RequestId requestId,
            QString collectionUid,
            QSharedPointer<QStringList> facets,
            AoidePagination pagination);

    void slotCreatePlaylist(
            mixxx::network::RequestId requestId,
            AoidePlaylist playlist);
    void slotDeletePlaylist(
            mixxx::network::RequestId requestId,
            QString playlistUid);
    void slotLoadPlaylistBriefs(
            mixxx::network::RequestId requestId);

  signals:
    void networkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

    void listCollectionsResult(
            mixxx::network::RequestId requestId,
            QVector<AoideCollectionEntity> result);

    void createCollectionResult(
            mixxx::network::RequestId requestId,
            AoideEntityHeader result);

    void updateCollectionResult(
            mixxx::network::RequestId requestId,
            AoideEntityHeader result);

    void deleteCollectionResult(
            mixxx::network::RequestId requestId);

    void replaceTracksResult(
            mixxx::network::RequestId requestId,
            QJsonObject result);

    void relocateTracksResult(
            mixxx::network::RequestId requestId);

    void purgeTracksResult(
            mixxx::network::RequestId requestId);

    void listTagsFacetsResult(
            mixxx::network::RequestId requestId,
            QVector<AoideTagFacetCount> result);

    void listTagsResult(
            mixxx::network::RequestId requestId,
            QVector<AoideTagCount> result);

    void createPlaylistResult(
            mixxx::network::RequestId requestId,
            AoidePlaylistBriefEntity result);
    void deletePlaylistResult(
            mixxx::network::RequestId requestId);
    void loadPlaylistBriefsResult(
            mixxx::network::RequestId requestId,
            QVector<AoidePlaylistBriefEntity> result);

  private slots:
    void onShutdownNetworkReplyFinished();

    void onListCollectionsNetworkReplyFinished();
    void onCreateCollectionNetworkReplyFinished();
    void onUpdateCollectionNetworkReplyFinished();
    void onDeleteCollectionNetworkReplyFinished();

    void onReplaceTracksNetworkReplyFinished();
    void onRelocateTracksNetworkReplyFinished();
    void onPurgeTracksNetworkReplyFinished();

    void onListTagsFacetsNetworkReplyFinished();
    void onListTagsNetworkReplyFinished();

    void onCreatePlaylistNetworkReplyFinished();
    void onDeletePlaylistNetworkReplyFinished();
    void onLoadPlaylistBriefsNetworkReplyFinished();

    void onNetworkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

  private:
    QUrl resourceUrl(const QString& resourcePath) const;

    const QUrl m_baseUrl;

    const Settings m_settings;

    const QPointer<QNetworkAccessManager> m_networkAccessManager;

    // Customizable by subclassing
    const std::unique_ptr<HashtagCommentsTagger> m_hashtagCommentsTagger;

    const parented_ptr<network::JsonWebClient> m_jsonWebClient;

    enum class WriteRequestMethod {
        Put,
        Post,
        Delete,
    };
    typedef void (Gateway::*WriteRequestFinishedHandler)();
    struct WriteRequest {
        explicit WriteRequest(WriteRequestMethod methodArg)
                : method(methodArg),
                  finishedHandler(nullptr) {
        }
        WriteRequest(const WriteRequest&) = default;
        WriteRequest(WriteRequest&&) = default;

        WriteRequestMethod method;
        RequestId id;
        QString path;
        QUrlQuery query;
        QByteArray jsonContent;
        WriteRequestFinishedHandler finishedHandler;
    };
    QQueue<WriteRequest> m_queuedWriteRequests;
    RequestId m_pendingWriteRequestId;

    void enqueueWriteRequest(WriteRequest writeRequest);
    void finishWriteRequest(RequestId finishedWriteRequestId = RequestId());

    RequestId m_shutdownRequestId;
};

} // namespace aoide

} // namespace mixxx
