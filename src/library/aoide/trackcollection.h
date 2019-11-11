#pragma once

#include "library/externaltrackcollection.h"
#include "preferences/usersettings.h"

class TrackCollectionManager;

namespace mixxx {

class TrackLoader;

namespace aoide {

class Subsystem;
class Agent;

class TrackCollection : public ExternalTrackCollection {
    Q_OBJECT

  public:
    TrackCollection(
            TrackCollectionManager* trackCollectionManager,
            UserSettingsPointer userSettings);
    ~TrackCollection() override = default;

    void establishConnection() override;
    void finishPendingTasksAndDisconnect() override;

    ConnectionState connectionState() const override;

    QString name() const override;

    QString description() const override;

    void relocateDirectory(
            const QString& oldRootDir,
            const QString& newRootDir) override;

    void updateTracks(
            const QList<TrackRef>& updatedTracks) override;

    void purgeTracks(
            const QList<QString>& trackLocations) override;
    void purgeAllTracks(
            const QDir& rootDir) override;

    void saveTrack(
            const Track& track,
            ChangeHint changeHint) override;

    LibraryFeature* newLibraryFeature(
            Library* library,
            UserSettingsPointer userSettings) override;

  private slots:
    void onSubsystemConnected();
    void onSubsystemDisconnected();
    void onSubsystemCollectionsChanged(int flags);

  private:
    TrackLoader* const m_trackLoader;

    Subsystem* const m_subsystem;
    Agent* const m_agent;

    ConnectionState m_connectionState;
};

} // namespace aoide

} // namespace mixxx
