#pragma once

#include <QList>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QVector>

#include "library/aoide/domain/track.h"
#include "network/requestid.h"
#include "track/track.h"
#include "track/trackref.h"

namespace mixxx {

class TrackLoader;

namespace aoide {

class Gateway;

class TrackReplacementScheduler : public QObject {
    Q_OBJECT

  public:
    TrackReplacementScheduler(
            Gateway* gateway,
            TrackLoader* trackLoader,
            QObject* parent = nullptr);
    ~TrackReplacementScheduler() override = default;

    // Connect slots after moveToThread()
    void connectSlots();

    void invokeReplaceTracks(
            QString collectionUid,
            QList<TrackRef> trackRefs);

    void invokeCancel();

  public slots:
    void slotReplaceTracks(
            QString collectionUid,
            QList<TrackRef> trackRefs);

    void slotCancel();

  signals:
    // total = queued + pending + succeeded + failed
    void progress(int queued, int pending, int succeeded, int failed);

  private slots:
    void /*TrackLoader*/ onTrackLoaded(
            TrackRef trackRef,
            TrackPointer trackPtr);

    void /*Gateway*/ onNetworkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

    void /*Gateway*/ onReplaceTracksResult(
            mixxx::network::RequestId requestId,
            QJsonObject result);

  private:
    void makeProgress();
    void emitProgress();

    const QPointer<Gateway> m_gateway;

    const QPointer<TrackLoader> m_trackLoader;

    // Requests for different collections
    QQueue<std::pair<QString, QList<TrackRef>>> m_deferredRequests;

    QString m_collectionUid;

    QQueue<TrackRef> m_queuedTrackRefs;

    QVector<TrackRef> m_loadingTrackRefs;

    bool isLoading(
            const TrackRef& trackRef) const;
    bool enterLoading(
            const TrackRef& trackRef);
    bool leaveLoading(
            const TrackRef& trackRef);

    QList<AoideTrack> m_bufferedRequests;

    QSet<network::RequestId> m_pendingRequests;

    int m_pendingCounter;
    int m_succeededCounter;
    int m_failedCounter;
};

} // namespace aoide

} // namespace mixxx
