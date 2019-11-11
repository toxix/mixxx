#pragma once

#include <QNetworkAccessManager>
#include <QStringList>
#include <QVector>

#include "library/aoide/domain/track.h"
#include "library/aoide/util.h"
#include "network/jsonwebtask.h"

namespace mixxx {

namespace aoide {

class SearchTracksTask : public network::JsonWebTask {
    Q_OBJECT

  public:
    SearchTracksTask(
            QNetworkAccessManager* networkAccessManager,
            QUrl baseUrl,
            const QString& collectionUid,
            const QJsonObject& baseQuery,
            const QStringList& searchTerms,
            const AoidePagination& pagination,
            QObject* parent = nullptr);
    ~SearchTracksTask() override = default;

  signals:
    void failed(
            network::JsonWebResponse response);
    void finished(
            QVector<AoideTrackEntity> result);

  protected:
    void connectSlots() override;

  private slots:
    void slotFinished(
            network::JsonWebResponse response);
};

} // namespace aoide

} // namespace mixxx
