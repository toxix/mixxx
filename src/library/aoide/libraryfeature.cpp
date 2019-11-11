#include "library/aoide/libraryfeature.h"

#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>

#include "library/aoide/settings.h"
#include "library/aoide/tracktablemodel.h"
#include "library/library.h"
#include "library/treeitem.h"
#include "sources/soundsourceproxy.h"
#include "util/assert.h"
#include "util/cmdlineargs.h"
#include "util/logger.h"
#include "widget/wlibrary.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide LibraryFeature");

const QString kInitialSearch = QStringLiteral("");

QString defaultPreparedQueriesFilePath(
        const UserSettingsPointer& settings) {
    auto filePath = Settings(settings).preparedQueriesFilePath();
    if (filePath.isEmpty()) {
        filePath = CmdlineArgs::Instance().getSettingsPath();
    }
    return filePath;
}

QJsonArray loadPreparedQueries(const QString& fileName) {
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isArray()) {
            return jsonDoc.array();
        } else {
            kLogger.warning()
                    << "Expected a JSON array with prepared queries and groups:"
                    << jsonDoc;
        }
    } else {
        kLogger.warning()
                << "Failed to open file:"
                << fileName;
    }
    return QJsonArray();
}

// Returns a null QString on success, otherwise an error message
QString savePreparedQueries(
        const QString& fileName,
        const QJsonArray& preparedQueries) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray jsonData =
                QJsonDocument(preparedQueries).toJson(QJsonDocument::Compact);
        const auto bytesWritten = file.write(jsonData);
        file.close();
        DEBUG_ASSERT(bytesWritten <= jsonData.size());
        if (bytesWritten >= jsonData.size()) {
            return QString(); // success
        }
        kLogger.warning()
                << "Failed to save prepared queries into file:"
                << fileName
                << file.errorString();
    } else {
        kLogger.warning()
                << "Failed to open file for writing:"
                << fileName
                << file.errorString();
    }
    DEBUG_ASSERT(!file.errorString().isNull());
    return file.errorString();
}

} // anonymous namespace

LibraryFeature::LibraryFeature(
        Library* library,
        UserSettingsPointer settings,
        Subsystem* subsystem)
        : ::LibraryFeature(library, settings),
          m_title(QStringLiteral("aoide")),
          m_icon(QStringLiteral(":/images/library/ic_library_aoide.svg")),
          m_preparedQueriesIcon(QStringLiteral(":/images/library/ic_library_tag-search-filter.svg")),
          m_playlistsIcon(QStringLiteral(":/images/library/ic_library_playlist.svg")),
          m_loadPreparedQueriesAction(new QAction(tr("Load prepared queries..."), this)),
          m_savePreparedQueriesAction(new QAction(tr("Save prepared queries..."), this)),
          m_refreshQueryResultsAction(new QAction(tr("Refresh query results"), this)),
          m_reloadPlaylistsAction(new QAction(tr("Reload playlists"), this)),
          m_createPlaylistAction(new QAction(tr("Create new playlist..."), this)),
          m_deletePlaylistAction(new QAction(tr("Delete playlist..."), this)),
          m_refreshPlaylistEntriesAction(new QAction(tr("Refresh playlist entries"), this)),
          m_subsystem(subsystem),
          m_trackTableModel(new TrackTableModel(this, library, subsystem)),
          m_childModel(this),
          m_previousSearch(kInitialSearch) {
    m_childModel.setRootItem(std::make_unique<TreeItem>(this));

    // Actions
    connect(m_loadPreparedQueriesAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotLoadPreparedQueries);
    connect(m_savePreparedQueriesAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotSavePreparedQueries);
    connect(m_refreshQueryResultsAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotRefreshQueryResults);
    connect(m_reloadPlaylistsAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotReloadPlaylists);
    connect(m_createPlaylistAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotCreatePlaylist);
    connect(m_deletePlaylistAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotDeletePlaylist);
    connect(m_refreshPlaylistEntriesAction,
            &QAction::triggered,
            this,
            &LibraryFeature::slotRefreshPlaylistEntries);

    // Subsystem
    connect(m_subsystem,
            &Subsystem::connected,
            this,
            &LibraryFeature::slotConnected);
    connect(m_subsystem,
            &Subsystem::disconnected,
            this,
            &LibraryFeature::slotDisconnected);
    connect(m_subsystem,
            &Subsystem::networkRequestFailed,
            this,
            &LibraryFeature::slotNetworkRequestFailed);
    connect(m_subsystem,
            &Subsystem::createPlaylistResult,
            this,
            &LibraryFeature::slotPlaylistCreated);
    connect(m_subsystem,
            &Subsystem::deletePlaylistResult,
            this,
            &LibraryFeature::slotPlaylistDeleted);
    connect(m_subsystem,
            &Subsystem::loadPlaylistBriefsResult,
            this,
            &LibraryFeature::slotLoadPlaylistBriefsResult);

    QString preparedQueriesFilePath =
            Settings(m_pConfig).preparedQueriesFilePath();
    if (!preparedQueriesFilePath.isEmpty()) {
        reloadPreparedQueries(preparedQueriesFilePath);
    }

    reloadPlaylists();

    kLogger.debug() << "Created instance" << this;
}

LibraryFeature::~LibraryFeature() {
    kLogger.debug() << "Destroying instance" << this;
}

QVariant LibraryFeature::title() {
    return m_title;
}

QIcon LibraryFeature::getIcon() {
    return m_icon;
}

void LibraryFeature::bindLibraryWidget(
        WLibrary* /*libraryWidget*/,
        KeyboardEventFilter* /*keyboard*/) {
}

void LibraryFeature::bindSidebarWidget(
        WLibrarySidebar* /*sidebarWidget*/) {
}

TreeItemModel* LibraryFeature::getChildModel() {
    return &m_childModel;
}

void LibraryFeature::activate() {
    emit showTrackModel(m_trackTableModel);
    emit enableCoverArtDisplay(true);
}

void LibraryFeature::activateChild(const QModelIndex& index) {
    const auto currentSearch = m_trackTableModel->searchText();
    if (!currentSearch.isNull()) {
        m_previousSearch = currentSearch;
    }
    auto preparedQuery = preparedQueryAt(index);
    if (preparedQuery.isEmpty()) {
        const auto playlistBriefEntity = playlistAt(index);
        if (playlistBriefEntity.isEmpty()) {
            // Nothing
            if (m_activeChildIndex != index) {
                // Initial activation
                m_activeChildIndex = index;
                m_trackTableModel->reset();
            }
        } else {
            // Activate playlist
            if (m_activeChildIndex != index) {
                // Initial activation
                m_activeChildIndex = index;
                // TODO: Populate table model with playlist entries
                m_trackTableModel->reset();
            }
        }
    } else {
        // Activate prepared query
        if ((m_activeChildIndex != index) ||
                m_trackTableModel->searchText().isNull()) {
            // Initial activation
            m_activeChildIndex = index;
            DEBUG_ASSERT(!m_previousSearch.isNull());
            m_trackTableModel->searchTracks(preparedQuery, m_previousSearch);
        }
        emit restoreSearch(m_trackTableModel->searchText());
    }
    activate();
    emit switchToView(m_title);
}

void LibraryFeature::reactivateChild() {
    auto activeIndex = m_activeChildIndex;
    m_activeChildIndex = QModelIndex();
    activateChild(activeIndex);
}

QJsonObject LibraryFeature::preparedQueryAt(
        const QModelIndex& index) const {
    if (!index.isValid()) {
        return QJsonObject();
    }
    auto parentIndex = index;
    while (parentIndex.parent().parent().isValid()) {
        parentIndex = parentIndex.parent();
    }
    if (parentIndex.row() != 0) {
        return QJsonObject();
    }
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    if (!item) {
        return QJsonObject();
    }
    return item->getData().toJsonObject();
}

AoidePlaylistBriefEntity LibraryFeature::playlistAt(
        const QModelIndex& index) const {
    if (!index.isValid()) {
        return AoidePlaylistBriefEntity();
    }
    auto parentIndex = index;
    while (parentIndex.parent().parent().isValid()) {
        parentIndex = parentIndex.parent();
    }
    if (parentIndex.row() != 1) {
        return AoidePlaylistBriefEntity();
    }
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    if (!item) {
        return AoidePlaylistBriefEntity();
    }
    int row = item->getData().toInt();
    if (row < 0 || row >= m_playlistBriefEntities.size()) {
        return AoidePlaylistBriefEntity();
    }
    return m_playlistBriefEntities.at(row);
}

namespace {

std::vector<std::unique_ptr<TreeItem>> buildPreparedQuerySubtreeModel(
        LibraryFeature* feature,
        const QJsonArray& jsonItems) {
    std::vector<std::unique_ptr<TreeItem>> treeItems;
    treeItems.reserve(jsonItems.size());
    for (auto i = 0; i < jsonItems.size(); ++i) {
        if (!jsonItems[i].isObject()) {
            kLogger.warning()
                    << "invalid JSON item"
                    << jsonItems[i];
            continue;
        }
        const auto& jsonItem = jsonItems[i].toObject();
        auto treeItem = std::make_unique<TreeItem>(
                feature,
                jsonItem.value("name").toString());
        treeItem->setToolTip(jsonItem.value("desc").toString());
        auto jsonType = jsonItem.value("@type").toString();
        if (jsonType == "query") {
            treeItem->setData(jsonItem);
        } else if (jsonType == "group") {
            // TODO: Check JsonValue
            auto jsonValue = jsonItem.value("items");
            if (jsonValue.isArray()) {
                auto childItems = buildPreparedQuerySubtreeModel(feature, jsonValue.toArray());
                for (auto&& childItem : childItems) {
                    treeItem->appendChild(std::move(childItem));
                }
            } else {
                kLogger.warning()
                        << "Group"
                        << treeItem->getLabel()
                        << "contains invalid items"
                        << jsonValue;
            }
        } else {
            kLogger.warning()
                    << "Unknown item type"
                    << jsonType;
        }
        treeItems.push_back(std::move(treeItem));
    }
    return treeItems;
}

std::vector<std::unique_ptr<TreeItem>> buildPlaylistSubtreeModel(
        LibraryFeature* feature,
        const QVector<AoidePlaylistBriefEntity>& playlistBriefEntities) {
    std::vector<std::unique_ptr<TreeItem>> treeItems;
    treeItems.reserve(playlistBriefEntities.size());
    for (int i = 0; i < playlistBriefEntities.size(); ++i) {
        const auto playlistBrief = playlistBriefEntities[i].body();
        auto label = QString("%1 (%2)")
                             .arg(playlistBrief.name())
                             .arg(playlistBrief.entries().tracksCount());
        auto treeItem = std::make_unique<TreeItem>(
                feature,
                std::move(label),
                i);
        treeItem->setToolTip(playlistBrief.description());
        treeItems.push_back(std::move(treeItem));
    }
    return treeItems;
}

} // anonymous namespace

void LibraryFeature::rebuildChildModel() {
    TreeItem* rootItem = m_childModel.getRootItem();
    VERIFY_OR_DEBUG_ASSERT(rootItem) {
        return;
    }
    m_childModel.removeRows(0, rootItem->childRows());
    auto preparedQueriesRoot = std::make_unique<TreeItem>(
            this,
            tr("Prepared Queries"));
    preparedQueriesRoot->setIcon(m_preparedQueriesIcon);
    {
        auto childItems =
                buildPreparedQuerySubtreeModel(this, m_preparedQueries);
        for (auto&& childItem : childItems) {
            preparedQueriesRoot->appendChild(std::move(childItem));
        }
    }
    auto playlistsRoot = std::make_unique<TreeItem>(
            this,
            tr("Playlists"));
    playlistsRoot->setIcon(m_playlistsIcon);
    {
        auto childItems =
                buildPlaylistSubtreeModel(this, m_playlistBriefEntities);
        for (auto&& childItem : childItems) {
            playlistsRoot->appendChild(std::move(childItem));
        }
    }
    QList<TreeItem*> rootRows;
    rootRows.reserve(2);
    rootRows.append(preparedQueriesRoot.release());
    rootRows.append(playlistsRoot.release());
    m_childModel.insertTreeItemRows(rootRows, 0);
}

void LibraryFeature::onRightClick(
        const QPoint& globalPos) {
    // TODO
    Q_UNUSED(globalPos);
}

void LibraryFeature::onRightClickChild(
        const QPoint& globalPos,
        QModelIndex index) {
    kLogger.debug() << "onRightClickChild" << index;
    VERIFY_OR_DEBUG_ASSERT(index.isValid()) {
        return;
    }
    const auto parentIndex = index.parent();
    DEBUG_ASSERT(parentIndex.isValid());
    if (!parentIndex.parent().isValid()) {
        // 1st level
        DEBUG_ASSERT(index.column() == 0);
        switch (index.row()) {
        case 0: {
            // Prepared queries
            QMenu menu;
            menu.addAction(m_loadPreparedQueriesAction);
            menu.addAction(m_savePreparedQueriesAction);
            menu.exec(globalPos);
            return;
        }
        case 1: {
            // Playlists
            QMenu menu;
            menu.addAction(m_reloadPlaylistsAction);
            menu.addSeparator();
            m_createPlaylistAction->setEnabled(
                    m_subsystem &&
                    m_subsystem->isConnected() &&
                    !m_createPlaylistRequestId.isValid());
            menu.addAction(m_createPlaylistAction);
            m_deletePlaylistAction->setEnabled(false);
            menu.addAction(m_deletePlaylistAction);
            menu.exec(globalPos);
            return;
        }
        default:
            DEBUG_ASSERT(!"unreachable");
        }
    }
    DEBUG_ASSERT(parentIndex.parent().isValid());
    if (!parentIndex.parent().parent().isValid()) {
        // 2nd level
        if (parentIndex.row() == 1) {
            // Playlist item
            DEBUG_ASSERT(index.column() == 0); // no nesting (yet)
            if (m_activeChildIndex != index) {
                activateChild(index);
            }
            QMenu menu;
            menu.addAction(m_refreshPlaylistEntriesAction);
            menu.addSeparator();
            m_createPlaylistAction->setEnabled(
                    m_subsystem &&
                    m_subsystem->isConnected() &&
                    !m_createPlaylistRequestId.isValid());
            menu.addAction(m_createPlaylistAction);
            m_deletePlaylistAction->setEnabled(
                    m_subsystem &&
                    m_subsystem->isConnected() &&
                    !m_deletePlaylistRequestId.isValid());
            menu.addAction(m_deletePlaylistAction);
            menu.exec(globalPos);
            return;
        }
    }
    // Prepared query item
    if (m_activeChildIndex != index) {
        activateChild(index);
    }
    auto query = preparedQueryAt(index);
    if (query.isEmpty()) {
        return;
    }
    QMenu menu;
    menu.addAction(m_refreshQueryResultsAction);
    menu.exec(globalPos);
}

void LibraryFeature::slotConnected() {
    reloadPlaylists();
}

void LibraryFeature::slotDisconnected() {
    reloadPlaylists();
}

void LibraryFeature::slotLoadPreparedQueries() {
    const auto msgBoxTitle =
            tr("aoide: Load Prepared Queries from File");
    const auto filePath = QFileDialog::getOpenFileName(
            nullptr,
            msgBoxTitle,
            defaultPreparedQueriesFilePath(m_pConfig),
            "*.json");
    if (filePath.isEmpty()) {
        kLogger.info()
                << "No file with prepared queries selected";
        return;
    }
    if (!reloadPreparedQueries(filePath)) {
        // TODO: Display more detailed error message
        QMessageBox(
                QMessageBox::Warning,
                msgBoxTitle,
                tr("Failed to load prepared queries.") +
                        QStringLiteral("\n\n") + filePath,
                QMessageBox::Close)
                .exec();
    }
}

void LibraryFeature::slotSavePreparedQueries() {
    const auto msgBoxTitle =
            tr("aoide: Save Prepared Queries into File");
    const auto filePath = QFileDialog::getSaveFileName(
            nullptr,
            msgBoxTitle,
            defaultPreparedQueriesFilePath(m_pConfig),
            "*.json");
    if (filePath.isEmpty()) {
        kLogger.info()
                << "No file for saving prepared queries selected";
        return;
    }
    const auto errorMessage =
            savePreparedQueries(filePath, m_preparedQueries);
    if (errorMessage.isNull()) {
        Settings(m_pConfig).setPreparedQueriesFilePath(filePath);
        QMessageBox(
                QMessageBox::Information,
                msgBoxTitle,
                tr("Saved prepared queries.") +
                        QStringLiteral("\n\n") + filePath,
                QMessageBox::Ok)
                .exec();
    } else {
        QMessageBox(
                QMessageBox::Warning,
                msgBoxTitle,
                tr("Failed to save prepared queries:") +
                        QChar(' ') + errorMessage +
                        QStringLiteral("\n\n") + filePath,
                QMessageBox::Close)
                .exec();
    }
}

bool LibraryFeature::reloadPreparedQueries(
        const QString& filePath) {
    auto preparedQueries = loadPreparedQueries(filePath);
    if (preparedQueries.isEmpty()) {
        kLogger.warning()
                << "Failed to load prepared queries from file:"
                << filePath;
        return false;
    }
    m_preparedQueries = preparedQueries;
    Settings(m_pConfig).setPreparedQueriesFilePath(filePath);
    // TODO: Only rebuild the subtree underneath the prepared queries
    // node instead of the whole child model
    rebuildChildModel();
    return true;
}

bool LibraryFeature::reloadPlaylists() {
    if (!m_subsystem || !m_subsystem->isConnected()) {
        m_playlistBriefEntities.clear();
        // TODO: Only rebuild the subtree underneath the prepared queries
        // node instead of the whole child model
        rebuildChildModel();
        return false;
    }
    if (m_loadPlaylistBriefsRequestId.isValid()) {
        kLogger.info()
                << "Discarding pending request"
                << m_loadPlaylistBriefsRequestId
                << "for loading playlists";
    }
    m_loadPlaylistBriefsRequestId =
            m_subsystem->invokeLoadPlaylistBriefs();
    return m_loadPlaylistBriefsRequestId.isValid();
}

void LibraryFeature::slotRefreshQueryResults() {
    reactivateChild();
}

void LibraryFeature::notifyNetworkRequestFailed(
        const QString& msgBoxTitle,
        const QString& textMessage,
        mixxx::network::RequestId requestId,
        QString errorMessage) {
    DEBUG_ASSERT(!msgBoxTitle.isEmpty());
    QString msgBoxText;
    if (!textMessage.isEmpty()) {
        msgBoxText = textMessage + QStringLiteral("\n\n");
    }
    msgBoxText +=
            tr("Request #") +
            QString::number(requestId) +
            QStringLiteral("\n") +
            errorMessage;
    QMessageBox(
            QMessageBox::Warning,
            msgBoxTitle,
            msgBoxText,
            QMessageBox::Close)
            .exec();
}

void LibraryFeature::slotNetworkRequestFailed(
        mixxx::network::RequestId requestId,
        QString errorMessage) {
    if (requestId == m_loadPlaylistBriefsRequestId) {
        m_loadPlaylistBriefsRequestId.reset();
        // Silently suppress this error that has already been logged
        return;
    }
    if (requestId == m_createPlaylistRequestId) {
        m_createPlaylistRequestId.reset();
        notifyNetworkRequestFailed(
                tr("aoide: Create New Playlist"),
                tr("Failed to create new playlist."),
                requestId,
                errorMessage);
        return;
    }
}

void LibraryFeature::slotReloadPlaylists() {
    reloadPlaylists();
}

void LibraryFeature::slotRefreshPlaylistEntries() {
    reactivateChild();
}

void LibraryFeature::slotCreatePlaylist() {
    if (m_createPlaylistRequestId.isValid()) {
        // still pending
        return;
    }
    if (!m_subsystem || !m_subsystem->isConnected()) {
        return;
    }
    bool ok = false;
    auto name = QInputDialog::getText(
            nullptr,
            tr("aoide: Create New Playlist"),
            tr("Enter name for new playlist:"),
            QLineEdit::Normal,
            tr("New Playlist"),
            &ok)
                        .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    AoidePlaylist playlist;
    playlist.setName(std::move(name));
    playlist.setDescription(QStringLiteral("Created by Mixxx"));
    playlist.setEntries(QJsonArray{}); // empty
    kLogger.info()
            << "Creating playlist"
            << playlist;
    m_createPlaylistRequestId = m_subsystem->invokeCreatePlaylist(
            std::move(playlist));
    DEBUG_ASSERT(m_createPlaylistRequestId.isValid());
}

void LibraryFeature::slotPlaylistCreated(
        mixxx::network::RequestId requestId,
        AoidePlaylistBriefEntity playlistBrief) {
    if (m_createPlaylistRequestId == requestId) {
        kLogger.info()
                << "Created playlist"
                << playlistBrief;
        m_createPlaylistRequestId.reset();
        reloadPlaylists();
    } else {
        // Silently ignore results for unknown requests
        if (m_createPlaylistRequestId.isValid()) {
            kLogger.info()
                    << "Request"
                    << requestId
                    << "for creating a playlist is still pending";
        }
    }
}

void LibraryFeature::slotDeletePlaylist() {
    if (m_deletePlaylistRequestId.isValid()) {
        // still pending
        return;
    }
    if (!m_subsystem || !m_subsystem->isConnected()) {
        return;
    }
    const auto playlistBriefEntity =
            playlistAt(m_activeChildIndex);
    if (playlistBriefEntity.isEmpty()) {
        return;
    }
    const auto playlistName = playlistBriefEntity.body().name();
    if (QMessageBox::Ok != QMessageBox(QMessageBox::Question, tr("aoide: Delete Playlist"), tr("Do your really want to delete this playlist?") + QStringLiteral("\n\n") + playlistName, QMessageBox::Ok | QMessageBox::Cancel).exec()) {
        return;
    }
    const auto playlistUid = playlistBriefEntity.header().uid();
    kLogger.info()
            << "Deleting playlist"
            << playlistUid
            << playlistName;
    m_deletePlaylistRequestId = m_subsystem->invokeDeletePlaylist(
            std::move(playlistUid));
    DEBUG_ASSERT(m_deletePlaylistRequestId.isValid());
}

void LibraryFeature::slotPlaylistDeleted(
        mixxx::network::RequestId requestId) {
    if (m_deletePlaylistRequestId == requestId) {
        kLogger.info()
                << "Deleted playlist";
        m_deletePlaylistRequestId.reset();
        reloadPlaylists();
    } else {
        // Silently ignore results for unknown requests
        if (m_createPlaylistRequestId.isValid()) {
            kLogger.info()
                    << "Request"
                    << requestId
                    << "for deleting a playlist is still pending";
        }
    }
}

void LibraryFeature::slotLoadPlaylistBriefsResult(
        mixxx::network::RequestId requestId,
        QVector<AoidePlaylistBriefEntity> result) {
    if (requestId == m_loadPlaylistBriefsRequestId) {
        m_loadPlaylistBriefsRequestId.reset();
        m_playlistBriefEntities = result;
        // TODO: Only rebuild the subtree underneath the playlists
        // node instead of the whole child model
        rebuildChildModel();
        return;
    } else {
        // Silently ignore results for unknown requests
        if (m_loadPlaylistBriefsRequestId.isValid()) {
            kLogger.info()
                    << "Request"
                    << requestId
                    << "for loading playlist briefs is still pending";
        }
    }
}

/*
class AddPlaylistTracksTask : public QObject {

  public:
    explicit AddPlaylistTracksTask(
            Subsystem* susbsystem,
            QList<QUrl> trackUrls)
            : m_resolveTask(subsystem->resolveTrackUrls(std::move(trackUrls), this)) {
        connect(m_resolveTask,
                &ResolveTracksByUrlTask::finished,
                this,
                &&AddPlaylistTracksTask::slotTrackUrlsResolved);
    }

  signals:
    void finished(
            QMap<QUrl, QString> addedTrackUrls,
            QList<QUrl> unresolvedTrackUrls);

  private slots:
    void slotTrackUrlsResolved(
            ResolveTracksByUrlTask::QMap_QUrl_QString resolvedTrackUrls,
            QList<QUrl> unresolvedTrackUrls) {
        m_resolveTask->deleteLater();
        m_unresolvedTrackUrls = std::move(m_unresolvedTrackUrls);
        m_resolvedTrackUrls = std::move(resolvedTrackUrls);
        emit finished(addedTrackUrls, unresolvedTrackUrls);
    }

  private:
    ResolveTracksByUrlTask* m_resolveTask;
    QList<QUrl> m_unresolvedTrackUrls;
    QMap<QUrl, QString> m_resolvedTrackUrls;
};
*/

bool LibraryFeature::dropAccept(QList<QUrl> urls, QObject* pSource) {
    if (urls.isEmpty() || !pSource) {
        return false;
    }
    if (!m_activeChildIndex.isValid()) {
        return false;
    }
    const auto playlist = playlistAt(m_activeChildIndex);
    if (playlist.isEmpty()) {
        return false;
    }
    QList<EncodedUrl> encodedUrls;
    {
        encodedUrls.reserve(urls.size());
        for (const auto& url : qAsConst(urls)) {
            DEBUG_ASSERT(dragMoveAccept(url));
            encodedUrls.append(EncodedUrl::fromUrl(url));
        }
    }
    kLogger.warning()
            << "TODO dropAccept"
            << encodedUrls;
    return !encodedUrls.isEmpty();
}

bool LibraryFeature::dragMoveAccept(QUrl url) {
    return SoundSourceProxy::isUrlSupported(url);
}

bool LibraryFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
    if (!index.isValid()) {
        return false;
    }
    return !playlistAt(index).isEmpty() &&
            dragMoveAccept(url);
}

} // namespace aoide

} // namespace mixxx
