#pragma once

#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>
#include <QThread>

#include "library/aoide/domain/collection.h"
#include "library/aoide/domain/playlist.h"
#include "library/aoide/domain/track.h"
#include "library/aoide/settings.h"
#include "library/aoide/util.h"
#include "network/requestid.h"
#include "track/trackref.h"

namespace mixxx {

class TrackLoader;

namespace aoide {

class Gateway;
class SearchTracksTask;
class ResolveTracksByUrlTask;
class TrackReplacementScheduler;

class Subsystem : public QObject {
    Q_OBJECT

  public:
    Subsystem(
            UserSettingsPointer userSettings,
            TrackLoader* trackLoader,
            QObject* parent = nullptr);
    ~Subsystem() override;

    void startUp();
    void shutDown();

    const Settings& settings() const {
        return m_settings;
    }

    bool isConnected() const {
        return m_gateway != nullptr;
    }

    const QVector<AoideCollectionEntity>& allCollections() const {
        return m_allCollections;
    }

    bool hasActiveCollection() const {
        return !m_activeCollection.header().uid().isEmpty();
    }
    const AoideCollectionEntity& activeCollection() const {
        return m_activeCollection;
    }
    void selectActiveCollection(
            const QString& collectionUid = QString());

    SearchTracksTask* searchTracks(
            const QJsonObject& baseQuery,
            const QStringList& searchTerms,
            const AoidePagination& pagination,
            QObject* parent = nullptr);
    ResolveTracksByUrlTask* resolveTracksByUrl(
            QList<QUrl> trackUrls,
            QObject* parent = nullptr);

    void invokeRefreshCollections();
    void invokeCreateCollection(
            AoideCollection collection);
    void invokeUpdateCollection(
            AoideCollectionEntity collectionEntity);
    void invokeDeleteCollection(
            QString collectionUid);

    typedef network::RequestId RequestId;

    // Not completely thread-safe despite "async" -> All functions
    // need to read the active collection member! As long as the
    // active collection is not modified concurrently invoking these
    // functions is safe.
    RequestId invokeReplaceTrack(
            const Track& track);
    RequestId invokeRelocateTracks(
            const QList<QPair<QString, QString>>& relocatedLocations);
    RequestId invokeRelocateAllTracks(
            const QDir& oldDir, const QDir& newDir);
    RequestId invokePurgeTracks(
            const QStringList& trackLocations);
    RequestId invokePurgeAllTracks(
            const QDir& rootDir);
    void invokeReplaceTracks(
            QList<TrackRef> trackRefs);

    RequestId invokeCreatePlaylist(
            AoidePlaylist playlist);
    RequestId invokeDeletePlaylist(
            QString playlistUid);
    RequestId invokeLoadPlaylistBriefs();

    enum CollectionsChangedFlags {
        ALL_COLLECTIONS = 0x01,
        ACTIVE_COLLECTION = 0x02,
    };

  signals:
    void connected();
    void disconnected();

    void collectionsChanged(int flags);

    void searchTracksResult(
            mixxx::network::RequestId requestId,
            QVector<AoideTrackEntity> result);

    void createPlaylistResult(
            mixxx::network::RequestId requestId,
            AoidePlaylistBriefEntity result);
    void deletePlaylistResult(
            mixxx::network::RequestId requestId);
    void loadPlaylistBriefsResult(
            mixxx::network::RequestId requestId,
            QVector<AoidePlaylistBriefEntity> result);

    void replacingTracksProgress(int queued, int pending, int succeeded, int failed);

    void networkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

  private slots:
    void onReadyReadStandardOutputFromProcess();
    void onReadyReadStandardErrorFromProcess();

    void /*Gateway*/ onListCollectionsResult(
            mixxx::network::RequestId requestId,
            QVector<AoideCollectionEntity> result);
    void /*Gateway*/ onCreateCollectionResult(
            mixxx::network::RequestId requestId,
            AoideEntityHeader result);
    void /*Gateway*/ onUpdateCollectionResult(
            mixxx::network::RequestId requestId,
            AoideEntityHeader result);
    void /*Gateway*/ onDeleteCollectionResult(
            mixxx::network::RequestId requestId);

  private:
    void connectProcess(
            QString endpointAddr);

    void startThread();
    void stopThread();

    const Settings m_settings;

    const QPointer<TrackLoader> m_trackLoader;

    QThread m_thread;

    QProcess m_process;

    QNetworkAccessManager* m_networkAccessManager;

    Gateway* m_gateway;

    TrackReplacementScheduler* m_trackReplacementScheduler;

    QVector<AoideCollectionEntity> m_allCollections;

    AoideCollectionEntity m_activeCollection;
};

} // namespace aoide

} // namespace mixxx
