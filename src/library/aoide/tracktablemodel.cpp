#include "library/aoide/tracktablemodel.h"

#include <QRegularExpression>

#include "library/aoide/coverartdelegate.h"
#include "library/aoide/task/searchtrackstask.h"
#include "library/aoide/tag/hashtagcommentstagger.h"
#include "library/bpmdelegate.h"
#include "library/dao/trackschema.h"
#include "library/library.h"
#include "library/locationdelegate.h"
#include "library/previewbuttondelegate.h"
#include "library/stardelegate.h"
#include "library/starrating.h"
#include "library/trackcollection.h"
#include "mixer/playerinfo.h"
#include "mixer/playermanager.h"
#include "track/bpm.h"
#include "track/keyutils.h"
#include "util/assert.h"
#include "util/duration.h"
#include "util/logger.h"
#include "widget/wlibrarytableview.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide TrackTableModel");

const char* kSettingsNamespace = "aoide";

const TrackTableModel::Item kEmptyItem;

const int kItemsPerPage = 250;

const QStringList kColumns = {
        LIBRARYTABLE_PREVIEW,
        LIBRARYTABLE_PLAYED,
        LIBRARYTABLE_COVERART,
        LIBRARYTABLE_ARTIST,
        LIBRARYTABLE_TITLE,
        LIBRARYTABLE_ALBUM,
        LIBRARYTABLE_ALBUMARTIST,
        LIBRARYTABLE_YEAR,
        LIBRARYTABLE_GENRE,
        LIBRARYTABLE_COMMENT,
        LIBRARYTABLE_COMPOSER,
        LIBRARYTABLE_GROUPING,
        LIBRARYTABLE_TRACKNUMBER,
        LIBRARYTABLE_FILETYPE,
        LIBRARYTABLE_LOCATION,
        LIBRARYTABLE_DURATION,
        LIBRARYTABLE_BITRATE,
        LIBRARYTABLE_BPM_LOCK,
        LIBRARYTABLE_BPM,
        LIBRARYTABLE_KEY,
        LIBRARYTABLE_REPLAYGAIN,
        LIBRARYTABLE_SAMPLERATE,
        LIBRARYTABLE_CHANNELS,
        LIBRARYTABLE_RATING,
        LIBRARYTABLE_DATETIMEADDED,
        LIBRARYTABLE_TIMESPLAYED,
};

} // anonymous namespace

TrackTableModel::TrackTableModel(
        QObject* parent,
        Library* library,
        Subsystem* subsystem)
        : QAbstractTableModel(parent),
          TrackModel(library->trackCollection().database(), kSettingsNamespace),
          m_library(library),
          m_subsystem(subsystem),
          m_genreTagger(subsystem->settings()),
          m_itemsPerPage(kItemsPerPage),
          m_pendingSearchTask(nullptr),
          m_canFetchMore(false),
          m_pendingRequestFirstRow(0),
          m_pendingRequestLastRow(0),
          m_previewDeckGroup(PlayerManager::groupForPreviewDeck(0)) {
    // Build a map from the column names to their indices, used by fieldIndex()
    m_columnCache.setColumns(kColumns);

    initHeaderData();

    connect(&PlayerInfo::instance(),
            &PlayerInfo::trackLoaded,
            this,
            &TrackTableModel::trackLoaded);

    trackLoaded(m_previewDeckGroup, PlayerInfo::instance().getTrackInfo(m_previewDeckGroup));

    kLogger.debug() << "Created instance" << this;
}

TrackTableModel::~TrackTableModel() {
    kLogger.debug() << "Destroying instance" << this;
}

TrackModel::CapabilitiesFlags TrackTableModel::getCapabilities() const {
    return TRACKMODELCAPS_NONE | TRACKMODELCAPS_ADDTOPLAYLIST | TRACKMODELCAPS_ADDTOCRATE | TRACKMODELCAPS_ADDTOAUTODJ | TRACKMODELCAPS_EDITMETADATA | TRACKMODELCAPS_LOADTODECK | TRACKMODELCAPS_LOADTOSAMPLER | TRACKMODELCAPS_LOADTOPREVIEWDECK | TRACKMODELCAPS_HIDE | TRACKMODELCAPS_RESETPLAYED;
}

bool TrackTableModel::setHeaderData(
        int section,
        Qt::Orientation orientation,
        const QVariant& value,
        int role) {
    VERIFY_OR_DEBUG_ASSERT(section >= 0) {
        return false;
    }
    VERIFY_OR_DEBUG_ASSERT(section < m_columnHeaders.size()) {
        return false;
    }
    if (orientation != Qt::Horizontal) {
        // We only care about horizontal headers.
        return false;
    }
    m_columnHeaders[section].header[role] = value;
    emit headerDataChanged(orientation, section, section);
    return true;
}

QVariant TrackTableModel::headerData(
        int section,
        Qt::Orientation orientation,
        int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        QVariant headerValue = m_columnHeaders.value(section).header.value(role);
        if (!headerValue.isValid()) {
            // Try EditRole if DisplayRole wasn't present
            headerValue = m_columnHeaders.value(section).header.value(Qt::EditRole);
        }
        if (!headerValue.isValid()) {
            headerValue = QVariant(section).toString();
        }
        return headerValue;
    } else if (role == TrackModel::kHeaderWidthRole && orientation == Qt::Horizontal) {
        QVariant widthValue = m_columnHeaders.value(section).header.value(role);
        if (!widthValue.isValid()) {
            return 50;
        }
        return widthValue;
    } else if (role == TrackModel::kHeaderNameRole && orientation == Qt::Horizontal) {
        return m_columnHeaders.value(section).header.value(role);
    } else if (role == Qt::ToolTipRole && orientation == Qt::Horizontal) {
        QVariant tooltip = m_columnHeaders.value(section).header.value(role);
        if (tooltip.isValid())
            return tooltip;
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void TrackTableModel::initHeaderData() {
    // Set the column heading labels, rename them for translations and have
    // proper capitalization

    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED,
            tr("Played"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_ARTIST,
            tr("Artist"),
            200);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_TITLE,
            tr("Title"),
            300);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_ALBUM,
            tr("Album"),
            200);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_ALBUMARTIST,
            tr("Album Artist"),
            100);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_GENRE,
            tr("Genre"),
            100);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_COMPOSER,
            tr("Composer"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_GROUPING,
            tr("Grouping"),
            10);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_YEAR,
            tr("Year"),
            40);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_COMMENT,
            tr("Comment"),
            250);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_DURATION,
            tr("Duration"),
            70);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_FILETYPE,
            tr("Type"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_NATIVELOCATION,
            tr("Location"),
            100);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_RATING,
            tr("Rating"),
            100);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_BITRATE,
            tr("Bitrate"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_BPM,
            tr("BPM"),
            70);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_TRACKNUMBER,
            tr("Track #"),
            10);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_DATETIMEADDED,
            tr("Date Added"),
            90);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_KEY,
            tr("Key"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK,
            tr("BPM Lock"),
            10);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW,
            tr("Preview"),
            50);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_COVERART,
            tr("Cover Art"),
            90);
    setHeaderProperties(ColumnCache::COLUMN_LIBRARYTABLE_REPLAYGAIN,
            tr("ReplayGain"),
            50);
}

void TrackTableModel::setHeaderProperties(
        ColumnCache::Column column,
        QString title,
        int defaultWidth) {
    int section = fieldIndex(column);
    if (section >= m_columnHeaders.size()) {
        m_columnHeaders.resize(section + 1);
    }
    m_columnHeaders[section].column = column;
    setHeaderData(section, Qt::Horizontal, m_columnCache.columnName(column), TrackModel::kHeaderNameRole);
    setHeaderData(section, Qt::Horizontal, title, Qt::DisplayRole);
    setHeaderData(section, Qt::Horizontal, defaultWidth, TrackModel::kHeaderWidthRole);
}

bool TrackTableModel::isColumnHiddenByDefault(int column) {
    return (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COMPOSER)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TRACKNUMBER)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_YEAR)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_GROUPING)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_NATIVELOCATION)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_ALBUMARTIST)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_REPLAYGAIN));
}

int TrackTableModel::fieldIndex(ColumnCache::Column column) const {
    return m_columnCache.fieldIndex(column);
}

int TrackTableModel::fieldIndex(const QString& fieldName) const {
    return m_columnCache.fieldIndex(fieldName);
}

QAbstractItemDelegate* TrackTableModel::delegateForColumn(const int i, QObject* pParent) {
    auto* pTableView = qobject_cast<WLibraryTableView*>(pParent);
    DEBUG_ASSERT(pTableView);

    if (i == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_RATING)) {
        return new StarDelegate(pTableView);
    } else if (i == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
        return new BPMDelegate(pTableView);
    } else if (PlayerManager::numPreviewDecks() > 0 && i == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW)) {
        return new PreviewButtonDelegate(pTableView, i);
    } else if (i == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_NATIVELOCATION)) {
        return new LocationDelegate(pTableView);
    } else if (i == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART)) {
        return new CoverArtDelegate(pTableView);
    }
    return nullptr;
}

void TrackTableModel::refreshCell(int row, int column) {
    QModelIndex coverIndex = index(row, column);
    emit dataChanged(coverIndex, coverIndex);
}

QVariant TrackTableModel::data(const QModelIndex& index, int role) const {
    //qDebug() << this << "data()";
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::CheckStateRole && role != Qt::ToolTipRole)) {
        return QVariant();
    }

    int row = index.row();
    int column = index.column();

    // This value is the value in its most raw form. It was looked up either
    // from the SQL table or from the cached track layer.
    QVariant value = dataValue(index, role);

    // Format the value based on whether we are in a tooltip, display, or edit
    // role
    switch (role) {
    case Qt::ToolTipRole:
    case Qt::DisplayRole:
        if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_DURATION)) {
            bool ok;
            const auto duration = value.toDouble(&ok);
            if (ok && duration >= 0) {
                value = mixxx::Duration::formatTime(duration, Duration::Precision::SECONDS);
            } else {
                value = QString();
            }
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED)) {
            if (value.canConvert(QMetaType::Int)) {
                value = QString("(%1)").arg(value.toInt());
            }
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_DATETIMEADDED)) {
            QDateTime gmtDate = value.toDateTime();
            gmtDate.setTimeSpec(Qt::UTC);
            value = gmtDate.toLocalTime();
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
            if (role == Qt::DisplayRole) {
                value = value.toDouble() == 0.0
                        ? "-"
                        : QString("%1").arg(value.toDouble(), 0, 'f', 1);
            }
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK)) {
            value = value.toBool();
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_YEAR)) {
            value = value.toString().left(4);
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_REPLAYGAIN)) {
            value = mixxx::ReplayGain::ratioToString(value.toDouble());
        } // Otherwise, just use the column value.

        break;
    case Qt::EditRole:
        if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
            value = value.toDouble();
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED)) {
            value = index.sibling(
                                 row, fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PLAYED))
                            .data()
                            .toBool();
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_RATING)) {
            if (value.canConvert(QMetaType::Int)) {
                value = qVariantFromValue(StarRating(value.toInt()));
            }
        }
        break;
    case Qt::CheckStateRole:
        if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED)) {
            bool played = index.sibling(
                                       row, fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PLAYED))
                                  .data()
                                  .toBool();
            value = played ? Qt::Checked : Qt::Unchecked;
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
            bool locked = index.sibling(
                                       row, fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK))
                                  .data()
                                  .toBool();
            value = locked ? Qt::Checked : Qt::Unchecked;
        }
        break;
    default:
        break;
    }
    return value;
}

bool TrackTableModel::setData(
        const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid()) {
        return false;
    }

    int row = index.row();
    if (row < 0 || row >= rowCount()) {
        return false;
    }

    int column = index.column();
    if (column < 0 || column >= columnCount()) {
        return false;
    }

    // Over-ride sets to TIMESPLAYED and re-direct them to PLAYED
    if (role == Qt::CheckStateRole) {
        QString val = value.toInt() > 0 ? QString("true") : QString("false");
        if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED)) {
            QModelIndex playedIndex = index.sibling(row, fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PLAYED));
            return setData(playedIndex, val, Qt::EditRole);
        } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
            QModelIndex bpmLockindex = index.sibling(row, fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK));
            return setData(bpmLockindex, val, Qt::EditRole);
        }
        return false;
    }

    TrackPointer pTrack = getTrack(index);
    if (!pTrack) {
        return false;
    }
    setTrackValueForColumn(pTrack, column, value);

    return true;
}

void TrackTableModel::setTrackValueForColumn(TrackPointer pTrack, int column, QVariant value) {
    kLogger.warning()
            << "TODO: setTrackValueForColumn"
            << pTrack->getId()
            << column
            << value;
}

Qt::ItemFlags TrackTableModel::flags(const QModelIndex& index) const {
    // TODO: Enable in-place editing
    //return readWriteFlags(index);
    return readOnlyFlags(index);
}

Qt::ItemFlags TrackTableModel::readWriteFlags(
        const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    // Enable dragging songs from this data model to elsewhere (like the
    // waveform widget to load a track into a Player).
    defaultFlags |= Qt::ItemIsDragEnabled;

    int column = index.column();

    if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_FILETYPE) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_NATIVELOCATION) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_DURATION) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BITRATE) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_DATETIMEADDED) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_REPLAYGAIN)) {
        return defaultFlags;
    } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED)) {
        return defaultFlags | Qt::ItemIsUserCheckable;
    } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK)) {
        return defaultFlags | Qt::ItemIsUserCheckable;
    } else if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM)) {
        // Allow checking of the BPM-locked indicator.
        defaultFlags |= Qt::ItemIsUserCheckable;
        // Disable editing of BPM field when BPM is locked
        bool locked = index.sibling(
                                   index.row(), fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK))
                              .data()
                              .toBool();
        return locked ? defaultFlags : defaultFlags | Qt::ItemIsEditable;
    } else {
        return defaultFlags | Qt::ItemIsEditable;
    }
}

Qt::ItemFlags TrackTableModel::readOnlyFlags(const QModelIndex& index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    // Enable dragging songs from this data model to elsewhere (like the
    // waveform widget to load a track into a Player).
    defaultFlags |= Qt::ItemIsDragEnabled;

    return defaultFlags;
}

int TrackTableModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    DEBUG_ASSERT(!parent.isValid());
    return m_columnHeaders.size();
}

int TrackTableModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    DEBUG_ASSERT(!parent.isValid());
    int rowCount;
    if (m_itemPages.isEmpty()) {
        rowCount = 0;
    } else {
        const auto& lastPage = m_itemPages.last();
        rowCount = lastPage.m_firstRow + lastPage.m_items.size();
    }
    return rowCount;
}

int TrackTableModel::findItemPageIndex(int row) const {
    DEBUG_ASSERT(row >= 0);
    if (row >= rowCount()) {
        return -1;
    }
    int lowerIndex = 0;
    int upperIndex = m_itemPages.size();
    while (lowerIndex < upperIndex) {
        if (lowerIndex == (upperIndex - 1)) {
            DEBUG_ASSERT(lowerIndex < m_itemPages.size());
            const auto& lowerItemPage = m_itemPages[lowerIndex];
            Q_UNUSED(lowerItemPage);
            DEBUG_ASSERT(lowerItemPage.m_firstRow <= row);
            DEBUG_ASSERT((row - lowerItemPage.m_firstRow) < lowerItemPage.m_items.size());
            return lowerIndex;
        }
        auto middleIndex = lowerIndex + (upperIndex - lowerIndex) / 2;
        DEBUG_ASSERT(middleIndex < m_itemPages.size());
        const auto& middleItemPage = m_itemPages[middleIndex];
        if (row < middleItemPage.m_firstRow) {
            upperIndex = middleIndex;
        } else {
            lowerIndex = middleIndex;
        }
    }
    DEBUG_ASSERT(!"unreachable");
    return -1;
}

const TrackTableModel::Item& TrackTableModel::rowItem(int row) const {
    const auto itemPageIndex = findItemPageIndex(row);
    VERIFY_OR_DEBUG_ASSERT((itemPageIndex >= 0) && (itemPageIndex < m_itemPages.size())) {
        // Not available
        return kEmptyItem;
    }
    const auto& itemPage = m_itemPages[itemPageIndex];
    DEBUG_ASSERT(row >= itemPage.m_firstRow);
    const auto pageRow = row - itemPage.m_firstRow;
    DEBUG_ASSERT(pageRow < itemPage.m_items.size());
    return itemPage.m_items[pageRow];
}

const TrackTableModel::Item& TrackTableModel::item(const QModelIndex& index) const {
    int row = index.row();
    if (row < 0 || row >= rowCount()) {
        return kEmptyItem;
    }
    return rowItem(row);
}

QVariant TrackTableModel::dataValue(const QModelIndex& index, int role) const {
    if (role != Qt::DisplayRole &&
            role != Qt::ToolTipRole &&
            role != Qt::EditRole) {
        return QVariant();
    }

    int row = index.row();
    if (row < 0 || row >= rowCount()) {
        return QVariant();
    }
    auto item = rowItem(row);

    int column = index.column();
    if (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW)) {
        // Special case for preview column. Return whether trackId is the
        // current preview deck track.
        if (role == Qt::ToolTipRole) {
            return QVariant();
        }
        if (!m_previewDeckTrackId.isValid()) {
            return false;
        }
        auto cachedRow = m_trackIdRowCache.value(m_previewDeckTrackId, -1);
        if (cachedRow >= 0) {
            return cachedRow == index.row();
        }
        return m_previewDeckTrackId == getTrackId(index);
    }
    if (column >= 0 && column < m_columnHeaders.size()) {
        switch (m_columnHeaders[column].column) {
        case ColumnCache::COLUMN_LIBRARYTABLE_ARTIST: {
            const auto& artists = item.body().artists();
            DEBUG_ASSERT(artists.size() <= 1);
            return artists.isEmpty() ? QString() : artists.first().name();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_TITLE: {
            const auto& titles = item.body().titles();
            DEBUG_ASSERT(titles.size() <= 1);
            return titles.isEmpty() ? QString() : titles.first().name();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_ALBUM: {
            const auto& titles = item.body().album().titles();
            DEBUG_ASSERT(titles.size() <= 1);
            return titles.isEmpty() ? QString() : titles.first().name();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_ALBUMARTIST: {
            const auto& artists = item.body().album().artists();
            DEBUG_ASSERT(artists.size() <= 1);
            return artists.isEmpty() ? QString() : artists.first().name();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_GENRE: {
            return m_genreTagger.importGenre(
                    item.body().tags().facetedTags(AoideTag::kFacetGenre));
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_COMMENT: {
            return HashtagCommentsTagger::importCommentFromTags(item.body().tags(), false);
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_DURATION: {
            return item.body().mediaSource().audioContent().durationMs() / 1000.0; // seconds
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_BITRATE: {
            return item.body().mediaSource().audioContent().bitRateBps() / 1000.0; // kbps
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_BPM: {
            const QVector<AoideBeatMarker> beatMarkers =
                    item.body().markers().beatMarkers().markers();
            DEBUG_ASSERT(beatMarkers.size() <= 1); // TODO: Handle multiple markers
            if (!beatMarkers.isEmpty()) {
                return beatMarkers.first().tempoBpm(Bpm::kValueUndefined);
            }
            break;
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK: {
            return item.body().markers().beatMarkers().locked();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_KEY: {
            const QVector<AoideKeyMarker> keyMarkers =
                    item.body().markers().keyMarkers().markers();
            DEBUG_ASSERT(keyMarkers.size() <= 1); // TODO: Handle multiple markers
            if (!keyMarkers.isEmpty()) {
                mixxx::track::io::key::ChromaticKey key =
                        keyMarkers.first().key();
                if (key != mixxx::track::io::key::INVALID) {
                    // Render this key with the user-provided notation.
                    return KeyUtils::keyToString(key);
                }
            }
            return QVariant();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_FILETYPE: {
            auto contentType = item.body().mediaSource().contentTypeName();
            if (contentType.startsWith("audio/")) {
                return contentType.right(contentType.size() - 6);
            } else {
                return contentType;
            }
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_SAMPLERATE: {
            return item.body().mediaSource().audioContent().sampleRateHz();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_NATIVELOCATION: {
            const auto url = item.body().mediaSource().uri().toUrl();
            return TrackFile::fromUrl(url).location();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_GROUPING: {
            const auto& groupingTags = item.body().tags().facetedTags(AoideTag::kFacetContentGroup);
            DEBUG_ASSERT(groupingTags.size() <= 1);
            return groupingTags.isEmpty() ? QString() : groupingTags.first().label();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_COMPOSER: {
            const auto& actors = item.body().actors(AoideActor::kRoleComposer);
            DEBUG_ASSERT(actors.size() <= 1);
            return actors.isEmpty() ? QString() : actors.first().name();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_CHANNELS: {
            return item.body().mediaSource().audioContent().channelCount();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_YEAR: {
            return item.body().release().releasedAt();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_REPLAYGAIN: {
            return item.body().mediaSource().audioContent().replayGain().getRatio();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_PLAYED: {
            break; // TODO
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_TIMESPLAYED: {
            return item.body().collection(m_collectionUid).playCount();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_DATETIMEADDED: {
            return item.body().collection(m_collectionUid).since();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_RATING: {
            return qVariantFromValue(item.body().starRating());
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_TRACKNUMBER: {
            return item.body().trackNumbers();
        }
        case ColumnCache::COLUMN_LIBRARYTABLE_INVALID:
        case ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW:
        case ColumnCache::COLUMN_LIBRARYTABLE_COVERART:
            break; // Skip
        default:
            kLogger.critical()
                    << "Unhandled column" << m_columnHeaders[column].column;
            DEBUG_ASSERT(!"unreachable");
            break;
        }
    }
    /*
    switch (role) {
    case ItemDataRole::Uid:
        return item.header().uid();
    case ItemDataRole::RevVersion:
        return item.header().revision().version();
    case ItemDataRole::RevTimeStamp:
        return item.header().revision().timeStamp();
    case ItemDataRole::EncoderName:
        return item.body().mediaSource().audioContent().encoder().name();
    case ItemDataRole::EncoderSettings:
        return item.body().mediaSource().audioContent().encoder().settings();
    default:
        DEBUG_ASSERT(!"TODO");
        return QVariant();
    }
    */
    return QVariant();
}

bool TrackTableModel::canFetchMore(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_canFetchMore;
}

void TrackTableModel::startNewSearch(
        const AoidePagination& pagination) {
    VERIFY_OR_DEBUG_ASSERT(!m_pendingSearchTask) {
        return;
    }
    m_pendingSearchTask = m_subsystem->searchTracks(
            m_baseQuery,
            m_searchTerms,
            pagination);
    DEBUG_ASSERT(m_pendingSearchTask);
    connect(m_pendingSearchTask,
            &SearchTracksTask::finished,
            this,
            &TrackTableModel::searchTracksFinished);
    m_pendingSearchTask->invokeStart();
}

void TrackTableModel::fetchMore(const QModelIndex& parent) {
    VERIFY_OR_DEBUG_ASSERT(canFetchMore(parent)) {
        return;
    }
    if (m_pendingSearchTask) {
        kLogger.debug()
                << "Can't fetch more rows while a search task is pending";
        return;
    }
    if (!m_canFetchMore) {
        kLogger.debug()
                << "Can't fetch more rows";
        return;
    }
    DEBUG_ASSERT(m_itemsPerPage > 0);
    AoidePagination pagination;
    pagination.offset = rowCount();
    pagination.limit = m_itemsPerPage;
    m_pendingRequestFirstRow = pagination.offset;
    m_pendingRequestLastRow = m_pendingRequestFirstRow + (pagination.limit - 1);
    startNewSearch(pagination);
}

namespace {

const QRegularExpression kRegexpWhitespace("\\s+");

} // anonymous namespace

void TrackTableModel::abortPendingSearch() {
    if (m_pendingSearchTask) {
        kLogger.debug()
                << "Aborting a pending search task";
        const auto pendingSearchTask = m_pendingSearchTask;
        m_pendingSearchTask = nullptr;
        pendingSearchTask->disconnect();
        pendingSearchTask->invokeAbort();
        pendingSearchTask->deleteLater();
    }
}

void TrackTableModel::reset() {
    abortPendingSearch();
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    m_collectionUid = QString();
    m_baseQuery = QJsonObject();
    m_searchText = QString();
    m_itemPages.clear();
    m_canFetchMore = false;
    m_pendingRequestFirstRow = 0;
    m_pendingRequestLastRow = 0;
    endRemoveRows();
    m_trackIdRowCache.clear();
}

void TrackTableModel::searchTracks(
        QJsonObject baseQuery,
        QString searchText) {
    if (!m_subsystem->hasActiveCollection()) {
        kLogger.warning()
                << "Search not available without an active collection";
        return;
    }
    abortPendingSearch();
    // TODO: Parse the query string
    auto searchTerms = searchText.split(kRegexpWhitespace, QString::SkipEmptyParts);
    DEBUG_ASSERT(m_itemsPerPage > 0);
    AoidePagination pagination;
    pagination.offset = 0;
    pagination.limit = m_itemsPerPage;
    m_collectionUid = m_subsystem->activeCollection().header().uid();
    m_baseQuery = baseQuery;
    m_searchText = searchText;
    m_searchTerms = searchTerms;
    m_canFetchMore = true;
    m_pendingRequestFirstRow = pagination.offset;
    m_pendingRequestLastRow = m_pendingRequestFirstRow + (pagination.limit - 1);
    startNewSearch(pagination);
}

void TrackTableModel::searchTracksFinished(
        QVector<Item> result) {
    auto* const pendingSearchTask = qobject_cast<SearchTracksTask*>(sender());
    DEBUG_ASSERT(pendingSearchTask == m_pendingSearchTask);
    pendingSearchTask->disconnect();
    pendingSearchTask->deleteLater();
    VERIFY_OR_DEBUG_ASSERT(m_pendingSearchTask) {
        return;
    }
    m_pendingSearchTask = nullptr;

    if ((m_pendingRequestFirstRow == 0) && (rowCount() > 0)) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_itemPages.clear();
        endRemoveRows();
    }
    DEBUG_ASSERT(m_pendingRequestFirstRow == rowCount());
    kLogger.debug()
            << "Received"
            << result.size()
            << "tracks from subsystem";
    if (result.isEmpty()) {
        // No more results available
        m_canFetchMore = false;
    } else {
        int firstRow = m_pendingRequestFirstRow;
        int lastRow = m_pendingRequestFirstRow + (result.size() - 1);
        if (lastRow < m_pendingRequestLastRow) {
            // No more results available
            m_canFetchMore = false;
        }
        beginInsertRows(QModelIndex(), firstRow, lastRow);
        m_itemPages += ItemPage(m_pendingRequestFirstRow, std::move(result));
        endInsertRows();
        if (m_trackIdRowCache.capacity() <= 0) {
            // Initially reserve some capacity
            m_trackIdRowCache.reserve(rowCount());
        }
    }
}

TrackRef TrackTableModel::getTrackRef(const QModelIndex& index) const {
    if (!index.isValid()) {
        return TrackRef();
    }
    return getTrackRefByRow(index.row());
}

TrackRef TrackTableModel::getTrackRefByRow(int row) const {
    VERIFY_OR_DEBUG_ASSERT(row < rowCount()) {
        return TrackRef();
    }
    const auto url = rowItem(row).body().mediaSource().uri().toUrl();
    const auto trackFile = TrackFile::fromUrl(url);
    return TrackRef::fromFileInfo(trackFile);
}

TrackPointer TrackTableModel::getTrack(const QModelIndex& index) const {
    const auto trackRef = getTrackRef(index);
    if (!trackRef.isValid()) {
        return TrackPointer();
    }
    DEBUG_ASSERT(m_library);
    auto track = m_library->trackCollection().getTrackByRef(trackRef);
    if (track) {
        m_trackIdRowCache.insert(track->getId(), index.row());
    }
    return track;
}

TrackId TrackTableModel::getTrackId(const QModelIndex& index) const {
    VERIFY_OR_DEBUG_ASSERT(index.isValid()) {
        return TrackId();
    }
    return getTrackIdByRow(index.row());
}

TrackId TrackTableModel::getTrackIdByRow(int row) const {
    const auto trackRef = getTrackRefByRow(row);
    if (!trackRef.isValid()) {
        return TrackId();
    }
    DEBUG_ASSERT(m_library);
    auto trackId = m_library->trackCollection().getTrackIdByRef(trackRef);
    if (trackId.isValid()) {
        m_trackIdRowCache.insert(trackId, row);
    }
    return trackId;
}

QString TrackTableModel::getTrackLocation(const QModelIndex& index) const {
    return getTrackRef(index).getLocation();
}

const QLinkedList<int> TrackTableModel::getTrackRows(TrackId trackId) const {
    // Each track is expected to appear only once, i.e. no duplicates!
    QLinkedList<int> rows;
    VERIFY_OR_DEBUG_ASSERT(trackId.isValid()) {
        return rows;
    }
    int row = m_trackIdRowCache.value(trackId, -1);
    if (row < 0) {
        // Not cached
        for (row = 0; row < rowCount(); ++row) {
            if (getTrackIdByRow(row) == trackId) {
                m_trackIdRowCache.insert(trackId, row);
                rows.append(row);
                return rows;
            }
        }
    }
    if (row >= 0 && row < rowCount()) {
        rows.append(row);
    }
    return rows;
}

void TrackTableModel::search(const QString& searchText, const QString& extraFilter) {
    DEBUG_ASSERT(extraFilter.isEmpty());
    m_searchText = searchText;
    select();
}

const QString TrackTableModel::currentSearch() const {
    return m_searchText;
}

bool TrackTableModel::isColumnInternal(int column) {
    return (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_URL)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_CUEPOINT)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_WAVESUMMARYHEX)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_SAMPLERATE)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_MIXXXDELETED)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_HEADERPARSED)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PLAYED)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_KEY_ID)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_CHANNELS)) ||
            (column == fieldIndex(ColumnCache::COLUMN_TRACKLOCATIONSTABLE_FSDELETED)) ||
            (PlayerManager::numPreviewDecks() == 0 &&
                    column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_SOURCE)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_TYPE)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_LOCATION)) ||
            (column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_HASH));
}

void TrackTableModel::select() {
    searchTracks(m_searchText);
}

void TrackTableModel::trackLoaded(QString group, TrackPointer pTrack) {
    if (group == m_previewDeckGroup) {
        // If there was a previously loaded track, refresh its rows so the
        // preview state will update.
        if (m_previewDeckTrackId.isValid()) {
            const int numColumns = columnCount();
            QLinkedList<int> rows = getTrackRows(m_previewDeckTrackId);
            m_previewDeckTrackId = TrackId(); // invalidate
            foreach (int row, rows) {
                QModelIndex left = index(row, 0);
                QModelIndex right = index(row, numColumns);
                emit dataChanged(left, right);
            }
        }
        m_previewDeckTrackId = pTrack ? pTrack->getId() : TrackId();
    }
}

} // namespace aoide

} // namespace mixxx
