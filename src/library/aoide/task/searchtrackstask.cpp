#include "library/aoide/task/searchtrackstask.h"

#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("aoide SearchTracksTask");

network::JsonWebRequest searchTracksRequest(
        const QString& collectionUid,
        const QJsonObject& baseQuery,
        const QStringList& searchTerms,
        const AoidePagination& pagination) {
    DEBUG_ASSERT(baseQuery.isEmpty() || baseQuery.value("@type").toString() == "query");

    QJsonObject searchParams;

    QJsonArray allFilters;
    auto baseFilter = baseQuery.value("filter").toObject();
    if (!baseFilter.isEmpty()) {
        allFilters += baseFilter;
    }
    for (auto&& searchTerm : searchTerms) {
        if (searchTerm.isEmpty()) {
            continue;
        }
        QJsonArray anyFilters;
        // Search for term in all string fields
        anyFilters += QJsonObject{{
                "phrase",
                QJsonArray{
                        QJsonArray{}, // any string field
                        QJsonArray{searchTerm}},
        }};
        // Search for term in all tag labels (both plain and faceted tags)
        anyFilters += QJsonObject{
                {
                        "tag",
                        QJsonObject{
                                // no facets = any faceted or plain tag
                                {"label", QJsonObject{{"contains", searchTerm}}}},
                }};
        allFilters += QJsonObject{{"any", anyFilters}};
    }
    if (!allFilters.isEmpty()) {
        searchParams.insert("filter", QJsonObject{{"all", allFilters}});
    }
    QString sort = baseQuery.value("sort").toString();
    QStringList sortFields = sort.split(",", QString::SkipEmptyParts);
    QJsonArray ordering;
    for (auto sortField : sortFields) {
        sortField = sortField.trimmed();
        QString direction;
        if (sortField.startsWith('+')) {
            direction = "asc";
            sortField = sortField.right(sortField.size() - 1).trimmed();
        } else if (sortField.startsWith('-')) {
            direction = "dsc";
            sortField = sortField.right(sortField.size() - 1).trimmed();
        } else {
            kLogger.warning()
                    << "Missing direction for sort field"
                    << sortField;
        }
        auto sortOrder = QJsonArray{
                sortField,
                direction,
        };
        ordering += sortOrder;
    }
    if (!ordering.isEmpty()) {
        searchParams.insert("ordering", ordering);
    }

    QUrlQuery query;
    if (!collectionUid.isEmpty()) {
        query.addQueryItem("collectionUid", collectionUid);
    }
    pagination.addToQuery(&query);
    return network::JsonWebRequest{
            network::HttpRequestMethod::Post,
            QStringLiteral("/tracks/search"),
            query,
            QJsonDocument(searchParams)};
}

} // anonymous namespace

namespace aoide {

SearchTracksTask::SearchTracksTask(
        QNetworkAccessManager* networkAccessManager,
        QUrl baseUrl,
        const QString& collectionUid,
        const QJsonObject& baseQuery,
        const QStringList& searchTerms,
        const AoidePagination& pagination,
        QObject* parent)
        : JsonWebTask(
                  networkAccessManager,
                  std::move(baseUrl),
                  searchTracksRequest(collectionUid, baseQuery, searchTerms, pagination),
                  parent) {
}

void SearchTracksTask::connectSlots() {
    JsonWebTask::connectSlots();
    connect(this,
            &network::JsonWebTask::finished,
            this,
            &SearchTracksTask::slotFinished);
}

void SearchTracksTask::slotFinished(
        network::JsonWebResponse response) {
    if (!response.isStatusCodeSuccess()) {
        emit failed(std::move(response));
        return;
    }
    DEBUG_ASSERT(response.statusCode == network::kHttpStatusCodeOk);

    VERIFY_OR_DEBUG_ASSERT(response.content.isArray()) {
        kLogger.warning()
                << "Invalid JSON content"
                << response.content;
        emit failed(std::move(response));
        return;
    }
    const auto jsonArray = response.content.array();

    QVector<AoideTrackEntity> result;
    result.reserve(jsonArray.size());
    for (const auto& jsonValue : jsonArray) {
        DEBUG_ASSERT(jsonValue.isArray());
        AoideTrackEntity trackEntity(jsonValue.toArray());
        AoideTrack track = trackEntity.body();
        trackEntity.setBody(std::move(track));
        result.append(std::move(trackEntity));
    }

    emit finished(result);
}

} // namespace aoide

} // namespace mixxx
