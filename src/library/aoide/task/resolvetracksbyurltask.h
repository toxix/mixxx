#pragma once

#include <QNetworkAccessManager>
#include <QList>
#include <QMap>
#include <QUrl>

#include "network/jsonwebtask.h"

namespace mixxx {

namespace aoide {

class ResolveTracksByUrlTask : public network::JsonWebTask {
    Q_OBJECT

  public:
    ResolveTracksByUrlTask(
            QNetworkAccessManager* networkAccessManager,
            QUrl baseUrl,
            const QString& collectionUid,
            QList<QUrl> trackUrls,
            QObject* parent = nullptr);
    ~ResolveTracksByUrlTask() override = default;

  signals:
    void failed(
            network::JsonWebResponse response);
    void finished(
            QMap<QUrl, QString> resolvedTrackUrls,
            QList<QUrl> unresolvedTrackUrls);

  private slots:
    void slotFinished(
            network::JsonWebResponse response);

  protected:
    void connectSlots() override;

  private:
    QList<QUrl> m_unresolvedTrackUrls;
};

} // namespace aoide

} // namespace mixxx
