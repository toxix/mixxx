#include "library/aoide/trackcollection.h"

#include "library/aoide/agent.h"
#include "library/aoide/libraryfeature.h"
#include "library/aoide/subsystem.h"
#include "library/trackcollectionmanager.h"
#include "library/trackloader.h"
#include "util/logger.h"

namespace mixxx {

namespace aoide {

const Logger kLogger("aoide TrackCollection");

TrackCollection::TrackCollection(
        TrackCollectionManager* trackCollectionManager,
        UserSettingsPointer userSettings)
        : ExternalTrackCollection(trackCollectionManager),
          m_trackLoader(new TrackLoader(trackCollectionManager, this)),
          m_subsystem(new Subsystem(userSettings, m_trackLoader, this)),
          m_agent(new Agent(m_subsystem, this)),
          m_connectionState(ConnectionState::Disconnected) {
    connect(m_subsystem,
            &Subsystem::connected,
            this,
            &TrackCollection::onSubsystemConnected);
    connect(m_subsystem,
            &Subsystem::disconnected,
            this,
            &TrackCollection::onSubsystemDisconnected);
    connect(m_subsystem,
            &Subsystem::collectionsChanged,
            this,
            &TrackCollection::onSubsystemCollectionsChanged);
    m_agent->connectSlots();
}

void TrackCollection::establishConnection() {
    VERIFY_OR_DEBUG_ASSERT(
            m_connectionState == ConnectionState::Disconnected) {
        return;
    }
    m_connectionState = ConnectionState::Connecting;
    m_subsystem->startUp();
}

void TrackCollection::finishPendingTasksAndDisconnect() {
    m_connectionState = ConnectionState::Disconnecting;
    m_subsystem->shutDown(); // synchronously
}

void TrackCollection::onSubsystemConnected() {
    if (m_connectionState == ConnectionState::Disconnecting) {
        // Disconnect requested while connecting
        return;
    }
    DEBUG_ASSERT(m_connectionState == ConnectionState::Connecting);
    // An active collection is required until fully connected!
    onSubsystemCollectionsChanged(Subsystem::ACTIVE_COLLECTION);
}

void TrackCollection::onSubsystemDisconnected() {
    DEBUG_ASSERT(m_connectionState == ConnectionState::Disconnecting);
    m_connectionState = ConnectionState::Disconnected;
    emit connectionStateChanged(m_connectionState);
}

void TrackCollection::onSubsystemCollectionsChanged(int flags) {
    Q_UNUSED(flags)
    if (m_subsystem->isConnected() && m_subsystem->hasActiveCollection()) {
        if (m_connectionState == ConnectionState::Connecting) {
            m_connectionState = ConnectionState::Connected;
            emit connectionStateChanged(m_connectionState);
        }
    } else {
        if (m_connectionState == ConnectionState::Connected) {
            m_connectionState = ConnectionState::Connecting;
            emit connectionStateChanged(m_connectionState);
        }
    }
}

ExternalTrackCollection::ConnectionState TrackCollection::connectionState() const {
    DEBUG_ASSERT(m_subsystem->isConnected() ||
            m_connectionState != ConnectionState::Connected);
    return m_connectionState;
}

QString TrackCollection::name() const {
    return tr("aoide");
}

QString TrackCollection::description() const {
    return tr("aoide Music Library");
}

void TrackCollection::relocateDirectory(
        const QString& oldRootDir,
        const QString& newRootDir) {
    kLogger.debug()
            << "Relocating directory:"
            << oldRootDir
            << "->"
            << newRootDir;
    m_subsystem->invokeRelocateAllTracks(oldRootDir, newRootDir);
}

void TrackCollection::updateTracks(
        const QList<TrackRef>& updatedTracks) {
    kLogger.debug()
            << "Updating tracks:"
            << updatedTracks;
    m_subsystem->invokeReplaceTracks(updatedTracks);
}

void TrackCollection::purgeTracks(
        const QList<QString>& trackLocations) {
    kLogger.debug()
            << "Purging tracks:"
            << trackLocations;
    m_subsystem->invokePurgeTracks(trackLocations);
}

void TrackCollection::purgeAllTracks(
        const QDir& rootDir) {
    kLogger.debug()
            << "Purging all tracks:"
            << rootDir;
    m_subsystem->invokePurgeAllTracks(rootDir);
}

void TrackCollection::saveTrack(
        const Track& track,
        ChangeHint /*changeHint*/) {
    DEBUG_ASSERT(track.getDateAdded().isValid());
    kLogger.debug()
            << "Saving track:"
            << track.getId()
            << track.getFileInfo();
    m_subsystem->invokeReplaceTrack(track);
}

::LibraryFeature* TrackCollection::newLibraryFeature(
        Library* library,
        UserSettingsPointer userSettings) {
    return new LibraryFeature(
            library,
            userSettings,
            m_subsystem);
}

} // namespace aoide

} // namespace mixxx
