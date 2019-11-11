#include "library/aoide/subsystem.h"

#include <QCoreApplication>
#include <QMetaObject>

#include "library/aoide/gateway.h"
#include "library/aoide/trackreplacementscheduler.h"
#include "library/trackloader.h"
#include "util/logger.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide Subsystem");

const QString kExecutableName = QStringLiteral("aoide");

const QString kDatabaseFileName = QStringLiteral("aoide.sqlite");

// The shutdown is delayed until all pending write requests have
// been finished. This timeout controls how long to wait for those
// pending write requests.
const int kProcessShutdownTimeoutMillis = 30000;

const QString kThreadName = QStringLiteral("aoide");

const QThread::Priority kThreadPriority = QThread::LowPriority;

const AoideCollectionEntity DEFAULT_COLLECTION_ENTITY = AoideCollectionEntity();

void startProcess(
        QProcess& process,
        const Settings& settings) {
    auto command = settings.command();
    if (command.isEmpty()) {
        // Try to load the executable from the settings folder first
        command = QDir(settings.getSettingsPath()).filePath(kExecutableName);
        if (!QFileInfo::exists(command)) {
            // ...otherwise try to load the executable from the application folder
            command = QDir(QCoreApplication::applicationDirPath()).filePath(kExecutableName);
        }
    }
    QFileInfo cmdFile(command);
    if (!cmdFile.exists()) {
        kLogger.warning()
                << "Executable file not found"
                << command;
        command = kExecutableName;
    }
    auto database = settings.database();
    if (database.isEmpty()) {
        database = QDir(settings.getSettingsPath()).filePath(kDatabaseFileName);
    }
    auto dbFile = QFileInfo(database);
    if (dbFile.exists()) {
        kLogger.info()
                << "Using existing database file"
                << database;
    } else {
        kLogger.info()
                << "Creating new database file"
                << database;
    }
    QStringList arguments;
    arguments += "--listen";
    arguments += settings.endpointAddress();
    QString verbosityArg = "-v"; // ERROR + WARN
    if (kLogger.infoEnabled()) {
        verbosityArg += 'v'; // + INFO
    }
    if (kLogger.debugEnabled()) {
        verbosityArg += 'v'; // + DEBUG
    }
    if (kLogger.traceEnabled()) {
        verbosityArg += 'v'; // + TRACE
    }
    arguments += verbosityArg;
    arguments += database;
    kLogger.info()
            << "Starting process"
            << command
            << arguments;
    process.start(command, arguments);
}

bool findCollectionByUid(
        const QVector<AoideCollectionEntity>& allCollections,
        const QString& collectionUid,
        AoideCollectionEntity* result = nullptr) {
    for (const auto& collection : allCollections) {
        if (collectionUid == collection.header().uid()) {
            if (result) {
                *result = collection;
            }
            return true;
        }
    }
    if (result) {
        *result = DEFAULT_COLLECTION_ENTITY;
    }
    return false;
}

} // namespace

Subsystem::Subsystem(
        UserSettingsPointer userSettings,
        TrackLoader* trackLoader,
        QObject* parent)
        : QObject(parent),
          m_settings(std::move(userSettings)),
          m_trackLoader(trackLoader),
          m_networkAccessManager(nullptr),
          m_gateway(nullptr),
          m_trackReplacementScheduler(nullptr) {
    DEBUG_ASSERT(!isConnected());
}

Subsystem::~Subsystem() {
    DEBUG_ASSERT(m_process.state() == QProcess::NotRunning);
}

void Subsystem::startUp() {
    connect(&m_process,
            &QProcess::readyReadStandardOutput,
            this,
            &Subsystem::onReadyReadStandardOutputFromProcess);
    connect(&m_process,
            &QProcess::readyReadStandardError,
            this,
            &Subsystem::onReadyReadStandardErrorFromProcess);
    startProcess(m_process, m_settings);
}

void Subsystem::onReadyReadStandardOutputFromProcess() {
    VERIFY_OR_DEBUG_ASSERT(!isConnected()) {
        kLogger.warning()
                << "Received unexpected output from process:"
                << QString::fromLocal8Bit(m_process.readAllStandardOutput());
        return;
    }
    const auto lines = QString::fromLocal8Bit(m_process.readAllStandardOutput()).split('\n', QString::SkipEmptyParts);
    for (auto&& line : lines) {
        auto endpointAddress = line.trimmed();
        if (!endpointAddress.isEmpty()) {
            kLogger.info()
                    << "Received endpoint address"
                    << endpointAddress;
            connectProcess(endpointAddress);
            DEBUG_ASSERT(isConnected());
            startThread();
            emit connected();
            return;
        }
    }
}

void Subsystem::onReadyReadStandardErrorFromProcess() {
    // Forward stderr from process into log file
    Logging::write(m_process.readAllStandardError());
}

void Subsystem::connectProcess(
        QString endpointAddr) {
    DEBUG_ASSERT(!m_networkAccessManager);
    m_networkAccessManager = new QNetworkAccessManager;
    m_networkAccessManager->moveToThread(&m_thread);
    connect(&m_thread,
            &QThread::finished,
            m_networkAccessManager,
            &QObject::deleteLater);

    DEBUG_ASSERT(!m_gateway);
    m_gateway = new Gateway(
            m_settings.baseUrl(std::move(endpointAddr)),
            m_settings,
            m_networkAccessManager);
    m_gateway->moveToThread(&m_thread);
    connect(&m_thread,
            &QThread::finished,
            m_gateway,
            &QObject::deleteLater);
    m_gateway->connectSlots();

    DEBUG_ASSERT(!m_trackReplacementScheduler);
    m_trackReplacementScheduler = new TrackReplacementScheduler(
            m_gateway,
            m_trackLoader);
    m_trackReplacementScheduler->moveToThread(&m_thread);
    connect(&m_thread,
            &QThread::finished,
            m_trackReplacementScheduler,
            &QObject::deleteLater);
    m_trackReplacementScheduler->connectSlots();
}

void Subsystem::shutDown() {
    if (m_process.state() == QProcess::Running) {
        if (m_gateway) {
            kLogger.warning() << "Shutting down process...";
            m_gateway->invokeShutdown();
            if (!m_process.waitForFinished(kProcessShutdownTimeoutMillis)) {
                kLogger.warning() << "Failed to gracefully shut down the process";
            }
        } else {
            kLogger.warning() << "Unable to shut down the process gracefully";
        }
    }
    stopThread();
    emit disconnected();
}

void Subsystem::startThread() {
    kLogger.info() << "Starting thread";
    DEBUG_ASSERT(thread() == QThread::currentThread());
    m_thread.setObjectName(kThreadName);
    m_thread.start(kThreadPriority);
    connect(m_gateway,
            &Gateway::listCollectionsResult,
            this,
            &Subsystem::onListCollectionsResult);
    connect(m_gateway,
            &Gateway::createCollectionResult,
            this,
            &Subsystem::onCreateCollectionResult);
    connect(m_gateway,
            &Gateway::updateCollectionResult,
            this,
            &Subsystem::onUpdateCollectionResult);
    connect(m_gateway,
            &Gateway::deleteCollectionResult,
            this,
            &Subsystem::onDeleteCollectionResult);
    connect(m_trackReplacementScheduler,
            &TrackReplacementScheduler::progress,
            this,
            &Subsystem::replacingTracksProgress); // signal/signal pass-through
    connect(m_gateway,
            &Gateway::createPlaylistResult,
            this,
            &Subsystem::createPlaylistResult); // signal/signal pass-through
    connect(m_gateway,
            &Gateway::deletePlaylistResult,
            this,
            &Subsystem::deletePlaylistResult); // signal/signal pass-through
    connect(m_gateway,
            &Gateway::loadPlaylistBriefsResult,
            this,
            &Subsystem::loadPlaylistBriefsResult); // signal/signal pass-through
    connect(m_gateway,
            &Gateway::networkRequestFailed,
            this,
            &Subsystem::networkRequestFailed); // signal/signal pass-through
    invokeRefreshCollections();
}

void Subsystem::stopThread() {
    kLogger.info() << "Stopping thread";
    DEBUG_ASSERT(thread() == QThread::currentThread());
    m_trackReplacementScheduler->invokeCancel();
    m_thread.quit();
    m_thread.wait();
}

void Subsystem::selectActiveCollection(
        const QString& collectionUid) {
    DEBUG_ASSERT(thread() == QThread::currentThread());
    QString activeCollectionUidBefore = m_activeCollection.header().uid();
    findCollectionByUid(m_allCollections, collectionUid, &m_activeCollection);
    QString activeCollectionUidAfter = m_activeCollection.header().uid();
    if (activeCollectionUidBefore != activeCollectionUidAfter) {
        if (hasActiveCollection()) {
            // Only overwrite the settings if a different collection
            // has actually been selected!
            m_settings.setCollectionUid(activeCollectionUidAfter);
            kLogger.info()
                    << "Selected active collection:"
                    << m_activeCollection;
        }
        emit collectionsChanged(
                CollectionsChangedFlags::ACTIVE_COLLECTION);
    }
}
SearchTracksTask* Subsystem::searchTracks(
        const QJsonObject& baseQuery,
        const QStringList& searchTerms,
        const AoidePagination& pagination,
        QObject* parent) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    return m_gateway->searchTracks(
            m_activeCollection.header().uid(),
            baseQuery,
            searchTerms,
            pagination,
            parent);
}

ResolveTracksByUrlTask* Subsystem::resolveTracksByUrl(
        QList<QUrl> trackUrls,
        QObject* parent) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot resolve track URLs";
        return nullptr;
    }
    return m_gateway->resolveTracksByUrl(
            m_activeCollection.header().uid(),
            std::move(trackUrls),
            parent);
}

void Subsystem::invokeRefreshCollections() {
    m_gateway->invokeListCollections();
}

void Subsystem::invokeCreateCollection(
        AoideCollection collection) {
    m_gateway->invokeCreateCollection(collection);
}

void Subsystem::invokeUpdateCollection(
        AoideCollectionEntity collectionEntity) {
    m_gateway->invokeUpdateCollection(collectionEntity);
}

void Subsystem::invokeDeleteCollection(
        QString collectionUid) {
    m_gateway->invokeDeleteCollection(collectionUid);
}

Subsystem::RequestId Subsystem::invokeReplaceTrack(
        const Track& track) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot replace track"
                << track.getFileInfo();
        return RequestId();
    }
    auto collectionUid = m_activeCollection.header().uid();
    auto exportedTrack = m_gateway->exportTrack(collectionUid, track);
    return m_gateway->invokeReplaceTracks(
            collectionUid,
            QList<AoideTrack>{exportedTrack});
}

Subsystem::RequestId Subsystem::invokeRelocateTracks(
        QList<QPair<QString, QString>> const& relocatedLocations) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot relocate tracks"
                << relocatedLocations;
        return RequestId();
    }
    return m_gateway->invokeRelocateTracks(
            m_activeCollection.header().uid(),
            relocatedLocations);
}

Subsystem::RequestId Subsystem::invokeRelocateAllTracks(
        const QDir& oldDir,
        const QDir& newDir) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot relocate all tracks from"
                << oldDir
                << "to"
                << newDir;
        return RequestId();
    }
    return m_gateway->invokeRelocateAllTracks(
            m_activeCollection.header().uid(),
            oldDir,
            newDir);
}

Subsystem::RequestId Subsystem::invokePurgeTracks(
        const QStringList& trackLocations) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (trackLocations.isEmpty()) {
        return RequestId();
    }
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot purge tracks"
                << trackLocations;
        return RequestId();
    }
    return m_gateway->invokePurgeTracks(
            m_activeCollection.header().uid(),
            trackLocations);
}

Subsystem::RequestId Subsystem::invokePurgeAllTracks(
        const QDir& rootDir) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot purge tracks"
                << rootDir;
        return RequestId();
    }
    return m_gateway->invokePurgeAllTracks(
            m_activeCollection.header().uid(),
            rootDir);
}

void Subsystem::invokeReplaceTracks(
        QList<TrackRef> trackRefs) {
    // Accesses mutable member variables -> not thread-safe
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (trackRefs.isEmpty()) {
        return;
    }
    if (!hasActiveCollection()) {
        kLogger.warning()
                << "No active collection:"
                << "Cannot replace tracks"
                << trackRefs;
        return;
    }
    m_trackReplacementScheduler->invokeReplaceTracks(
            m_activeCollection.header().uid(),
            std::move(trackRefs));
}

Subsystem::RequestId Subsystem::invokeCreatePlaylist(
        AoidePlaylist playlist) {
    if (!m_gateway) {
        return RequestId();
    }
    return m_gateway->invokeCreatePlaylist(
            playlist);
}

Subsystem::RequestId Subsystem::invokeDeletePlaylist(
        QString playlistUid) {
    if (!m_gateway) {
        return RequestId();
    }
    return m_gateway->invokeDeletePlaylist(
            playlistUid);
}

Subsystem::RequestId Subsystem::invokeLoadPlaylistBriefs() {
    if (!m_gateway) {
        return RequestId();
    }
    return m_gateway->invokeLoadPlaylistBriefs();
}

void Subsystem::onListCollectionsResult(
        mixxx::network::RequestId /*requestId*/,
        QVector<AoideCollectionEntity> result) {
    m_allCollections = std::move(result);
    int changedFlags = CollectionsChangedFlags::ALL_COLLECTIONS;
    if (hasActiveCollection()) {
        if (!findCollectionByUid(m_allCollections, m_activeCollection.header().uid(), &m_activeCollection)) {
            // active collection has been reset
            kLogger.info()
                    << "Deselected active collection";
            changedFlags |= CollectionsChangedFlags::ACTIVE_COLLECTION;
        }
    } else {
        auto settingsCollectionUid = m_settings.collectionUid();
        for (auto&& collection : qAsConst(m_allCollections)) {
            if (collection.header().uid() == settingsCollectionUid) {
                m_activeCollection = collection;
                kLogger.info()
                        << "Reselected active collection:"
                        << m_activeCollection;
                changedFlags |= CollectionsChangedFlags::ACTIVE_COLLECTION;
                break; // exit loop
            }
        }
    }
    emit collectionsChanged(changedFlags);
}

void Subsystem::onCreateCollectionResult(
        mixxx::network::RequestId /*requestId*/,
        AoideEntityHeader /*result*/) {
    invokeRefreshCollections();
}

void Subsystem::onUpdateCollectionResult(
        mixxx::network::RequestId /*requestId*/,
        AoideEntityHeader /*result*/) {
    invokeRefreshCollections();
}

void Subsystem::onDeleteCollectionResult(
        mixxx::network::RequestId /*requestId*/) {
    invokeRefreshCollections();
}

} // namespace aoide

} // namespace mixxx
