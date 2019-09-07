#pragma once

#include <QMap>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <utility> // std::pair

#include "network/requestid.h"

namespace mixxx {

namespace network {

class RequestReplyManager: public QObject {
    Q_OBJECT

  public:
    explicit RequestReplyManager(
            QObject* parent = nullptr);
    ~RequestReplyManager() override;

    void afterRequestSent(
            RequestId requestId,
            QNetworkReply* pendingReply,
            int timeoutMillis);

    bool isRequestPending(RequestId requestId) const {
        return m_pendingRequests.contains(requestId);
    }

    void cancelRequest(
            RequestId requestId);
    void cancelAllRequests();

    // The second member of the result pair indicated if the
    // corresponding request is still pending. Otherwise it
    // has already been cancelled.
    std::pair<RequestId, bool> afterReplyReceived(
            QNetworkReply* finishedReply);

  protected:
    void timerEvent(QTimerEvent* event) override;

  private:
    RequestId killTimerForPendingReply(int timerId);

    struct PendingReply {
        RequestId requestId;
        int timerId;
    };
    QMap<RequestId, QNetworkReply*> m_pendingRequests;
    QMap<QNetworkReply*, PendingReply> m_pendingReplies;
};

} // namespace network

} // namespace mixxx
