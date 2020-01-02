#pragma once

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>

#include "network/httpstatuscode.h"
#include "network/requestreplymanager.h"

namespace mixxx {

namespace network {

class JsonWebClient : public QObject {
    Q_OBJECT

  public:
    static QNetworkRequest newRequest(
            const QUrl& url);

    static int isStatusCodeSuccess(
            int statusCode) {
        return statusCode >= 200 && statusCode < 300;
    }

    explicit JsonWebClient(
            QNetworkAccessManager* networkAccessManager,
            QObject* parent = nullptr);
    ~JsonWebClient() override = default;

    QNetworkAccessManager* accessNetwork(
            RequestId requestId);

    void afterRequestSent(
            RequestId requestId,
            QNetworkReply* reply,
            int timeoutMillis = 0);

    bool isRequestPending(
            RequestId requestId) const {
        return m_requestReplyManager.isRequestPending(requestId);
    }

    void cancelRequest(
            RequestId requestId);
    void cancelAllRequests();

    // Parse the results of a network reply
    std::pair<RequestId, HttpStatusCode> receiveReply(
            QNetworkReply* reply,
            QJsonDocument* jsonResponse = nullptr);

  signals:
    // Low-level network/infrastructure errors
    void networkRequestFailed(
            mixxx::network::RequestId requestId,
            QString errorMessage);

  private:
    const QPointer<QNetworkAccessManager> m_networkAccessManager;

    RequestReplyManager m_requestReplyManager;
};

} // namespace network

} // namespace mixxx
