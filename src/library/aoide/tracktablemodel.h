#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QList>
#include <QPointer>

#include "library/aoide/subsystem.h"
#include "library/aoide/tag/multigenretagger.h"
#include "library/columncache.h"
#include "library/trackmodel.h"
#include "track/track.h"

class Library;

namespace mixxx {

namespace aoide {

class TrackTableModel : public QAbstractTableModel, public TrackModel {
    Q_OBJECT

  public:
    typedef AoideTrackEntity Item;

    TrackTableModel(
            QObject* parent,
            Library* library,
            Subsystem* subsystem);
    ~TrackTableModel() override;

    const Item& item(const QModelIndex& index) const;

    ///////////////////////////////////////////////////////
    // Inherited from QAbstractTableModel
    ///////////////////////////////////////////////////////

    int columnCount(const QModelIndex& parent = QModelIndex()) const final;

    int rowCount(const QModelIndex& parent = QModelIndex()) const final;

    bool setHeaderData(
            int section,
            Qt::Orientation orientation,
            const QVariant& value,
            int role = Qt::DisplayRole) final;
    QVariant headerData(
            int section,
            Qt::Orientation orientation,
            int role = Qt::DisplayRole) const final;

    Qt::ItemFlags flags(const QModelIndex& index) const final;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const final;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) final;

    bool canFetchMore(const QModelIndex& parent) const final;
    void fetchMore(const QModelIndex& parent) final;

    const QString& searchText() const {
        return m_searchText;
    }

    ///////////////////////////////////////////////////////
    // Inherited from TrackModel
    ///////////////////////////////////////////////////////

    CapabilitiesFlags getCapabilities() const final;

    int fieldIndex(const QString& fieldName) const final;

    // Deserialize and return the track at the given QModelIndex in this result
    // set.
    TrackPointer getTrack(const QModelIndex& index) const final;

    // Gets the on-disk location of the track at the given location
    // with Qt separator "/".
    // Use QDir::toNativeSeparators() before displaying this to a user.
    QString getTrackLocation(const QModelIndex& index) const final;

    // Gets the track ID of the track at the given QModelIndex
    TrackId getTrackId(const QModelIndex& index) const final;

    // Gets the rows of the track in the current result set. Returns an
    // empty list if the track ID is not present in the result set.
    const QLinkedList<int> getTrackRows(TrackId trackId) const final;

    void search(const QString& searchText, const QString& extraFilter = QString()) final;
    const QString currentSearch() const final;

    bool isColumnInternal(int column) final;

    // if no header state exists, we may hide some columns so that the user can
    // reactivate them
    bool isColumnHiddenByDefault(int column) final;

    void select() final;

    QAbstractItemDelegate* delegateForColumn(const int i, QObject* pParent) final;

    /*
    const QList<int>& showableColumns() const { return m_emptyColumns; }
    const QList<int>& searchColumns() const { return m_emptyColumns; }

    void removeTracks(const QModelIndexList& indices) {
        Q_UNUSED(indices);
    }
    void hideTracks(const QModelIndexList& indices) {
        Q_UNUSED(indices);
    }
    void unhideTracks(const QModelIndexList& indices) {
        Q_UNUSED(indices);
    }
    void purgeTracks(const QModelIndexList& indices) {
        Q_UNUSED(indices);
    }
    int addTracks(const QModelIndex& index, const QList<QString>& locations) {
        Q_UNUSED(index);
        Q_UNUSED(locations);
        return 0;
    }
    void moveTrack(const QModelIndex& sourceIndex,
                           const QModelIndex& destIndex) {
        Q_UNUSED(sourceIndex);
        Q_UNUSED(destIndex);
    }
    bool isLocked() {
        return false;
    }

    int defaultSortColumn() const {
        return m_iDefaultSortColumn;
    }

    Qt::SortOrder defaultSortOrder() const {
        return m_eDefaultSortOrder;
    }

    void setDefaultSort(int sortColumn, Qt::SortOrder sortOrder) {
        m_iDefaultSortColumn = sortColumn;
        m_eDefaultSortOrder = sortOrder;
    }

    bool isColumnSortable(int column) {
        Q_UNUSED(column);
        return true;
    }

    SortColumnId sortColumnIdFromColumnIndex(int index) {
        Q_UNUSED(index);
        return SORTCOLUMN_INVALID;

    }

    int columnIndexFromSortColumnId(TrackModel::SortColumnId sortColumn) {
        Q_UNUSED(sortColumn);
        return -1;
    }
    */

  signals:
    void doubleClicked(
            TrackPointer track);
    void rightClickPressed(
            TrackPointer track);
    void rightClickReleased();

  public slots:
    void reset();

    void searchTracks(
            QString searchText) {
        searchTracks(m_baseQuery, searchText);
    }
    void searchTracks(
            QJsonObject baseQuery,
            QString searchText);

  private slots:
    void searchTracksFinished(
            QVector<Item> result);
    void trackLoaded(QString group, TrackPointer pTrack);
    void refreshCell(int row, int column);

  protected:
    // Use this if you want a model that is read-only.
    virtual Qt::ItemFlags readOnlyFlags(const QModelIndex& index) const;
    // Use this if you want a model that can be changed
    virtual Qt::ItemFlags readWriteFlags(const QModelIndex& index) const;

  private:
    void initHeaderData();
    void setHeaderProperties(
            ColumnCache::Column column,
            QString title,
            int defaultWidth);

    int fieldIndex(ColumnCache::Column column) const;

    const Item& rowItem(int row) const;
    QVariant dataValue(const QModelIndex& index, int role = Qt::DisplayRole) const;

    TrackRef getTrackRef(const QModelIndex& index) const;

    TrackId getTrackIdByRow(int row) const;
    TrackRef getTrackRefByRow(int row) const;

    void setTrackValueForColumn(TrackPointer pTrack, int column, QVariant value);

    void startNewSearch(
            const AoidePagination& pagination);
    void abortPendingSearch();

    const QPointer<Library> m_library;

    const QPointer<Subsystem> m_subsystem;

    MultiGenreTagger m_genreTagger;

    struct ColumnHeader {
        ColumnCache::Column column = ColumnCache::COLUMN_LIBRARYTABLE_INVALID;
        QHash</*role*/ int, QVariant> header;
    };
    QVector<ColumnHeader> m_columnHeaders;

    ColumnCache m_columnCache;

    int m_itemsPerPage;

    QJsonObject m_baseQuery;
    QString m_searchText;

    QString m_collectionUid;
    SearchTracksTask* m_pendingSearchTask;
    bool m_canFetchMore;
    int m_pendingRequestFirstRow;
    int m_pendingRequestLastRow;

    QStringList m_searchTerms;

    struct ItemPage {
        ItemPage(int firstRow, QVector<Item> items)
                : m_firstRow(firstRow),
                  m_items(std::move(items)) {
        }
        int m_firstRow;
        QVector<Item> m_items;
    };

    QList<ItemPage> m_itemPages;

    int findItemPageIndex(int row) const;

    // Each track is expected to appear only once, i.e. no duplicates!
    mutable QHash<TrackId, int> m_trackIdRowCache;

    QString m_previewDeckGroup;
    TrackId m_previewDeckTrackId;
};

} // namespace aoide

} // namespace mixxx
