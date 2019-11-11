#pragma once

#include <QAction>
#include <QJsonArray>
#include <QPointer>

#include "library/aoide/domain/playlist.h"
#include "library/libraryfeature.h"
#include "library/treeitemmodel.h"
#include "network/requestid.h"

class Library;

namespace mixxx {

namespace aoide {

class Subsystem;
class TrackTableModel;

class LibraryFeature : public ::LibraryFeature {
    Q_OBJECT

  public:
    LibraryFeature(
            Library* library,
            UserSettingsPointer settings,
            Subsystem* subsystem);
    ~LibraryFeature() override;

    QVariant title() override;
    QIcon getIcon() override;

    void bindLibraryWidget(
            WLibrary* libraryWidget,
            KeyboardEventFilter* keyboard) override;
    void bindSidebarWidget(
            WLibrarySidebar* sidebarWidget) override;
    TreeItemModel* getChildModel() override;

    bool hasTrackTable() override {
        return true;
    }

    bool dropAccept(QList<QUrl> urls, QObject* pSource) override;
    bool dragMoveAccept(QUrl url) override;
    bool dragMoveAcceptChild(const QModelIndex& index, QUrl url) override;

  public slots:
    void activate() override;
    void activateChild(
            const QModelIndex& index) override;
    void onRightClick(
            const QPoint& globalPos) override;
    void onRightClickChild(
            const QPoint& globalPos,
            QModelIndex index) override;

  private slots:
    void slotConnected();
    void slotDisconnected();

    void slotLoadPreparedQueries();
    void slotSavePreparedQueries();

    void slotRefreshQueryResults();

    void slotReloadPlaylists();
    void slotLoadPlaylistBriefsResult(
            mixxx::network::RequestId requestId,
            QVector<AoidePlaylistBriefEntity> result);

    void slotRefreshPlaylistEntries();
    void slotCreatePlaylist();
    void slotPlaylistCreated(
            mixxx::network::RequestId requestId,
            AoidePlaylistBriefEntity playlistBrief);
    void slotDeletePlaylist();
    void slotPlaylistDeleted(
            mixxx::network::RequestId requestId);

    void slotNetworkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

   void reactivateChild();

  private:
    const QString m_title;
    const QIcon m_icon;

    const QIcon m_preparedQueriesIcon;
    const QIcon m_playlistsIcon;

    QAction* const m_loadPreparedQueriesAction;
    QAction* const m_savePreparedQueriesAction;

    QAction* const m_refreshQueryResultsAction;

    QAction* const m_reloadPlaylistsAction;
    QAction* const m_createPlaylistAction;
    QAction* const m_deletePlaylistAction;

    QAction* const m_refreshPlaylistEntriesAction;

    const QPointer<Subsystem> m_subsystem;

    TrackTableModel* const m_trackTableModel;

    TreeItemModel m_childModel;

    QJsonArray m_preparedQueries;

    QVector<AoidePlaylistBriefEntity> m_playlistBriefEntities;

    bool reloadPreparedQueries(const QString& filePath);
    bool reloadPlaylists();

    void rebuildChildModel();

    void notifyNetworkRequestFailed(
          const QString& msgBoxTitle,
          const QString& textMessage,
          mixxx::network::RequestId requestId,
          QString errorMessage);

    QJsonObject preparedQueryAt(
            const QModelIndex& index) const;
    AoidePlaylistBriefEntity playlistAt(
            const QModelIndex& index) const;

    QModelIndex m_activeChildIndex;

    QString m_previousSearch;

    mixxx::network::RequestId m_createPlaylistRequestId;
    mixxx::network::RequestId m_deletePlaylistRequestId;
    mixxx::network::RequestId m_loadPlaylistBriefsRequestId;
};

} // namespace aoide

} // namespace mixxx
