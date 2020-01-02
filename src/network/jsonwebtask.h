#pragma once

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrlQuery>

#include "network/httpstatuscode.h"

namespace mixxx {

namespace network {

enum class HttpRequestMethod {
    Get,
    Put,
    Post,
    Delete,
};

struct JsonWebRequest {
  public:
    JsonWebRequest() = delete;
    JsonWebRequest(const JsonWebRequest&) = default;
    JsonWebRequest(JsonWebRequest&&) = default;

    HttpRequestMethod method;
    QString path;
    QUrlQuery query;
    QJsonDocument content;
};

struct JsonWebResponse {
  public:
    static void registerMetaType();

    JsonWebResponse()
            : statusCode(kHttpStatusCodeInvalid) {
    }
    JsonWebResponse(
            HttpStatusCode statusCode,
            QJsonDocument content)
            : statusCode(statusCode),
              content(content) {
    }
    JsonWebResponse(const JsonWebResponse&) = default;
    JsonWebResponse(JsonWebResponse&&) = default;

    bool isStatusCodeSuccess() const {
        return statusCode >= 200 && statusCode < 300;
    }

    HttpStatusCode statusCode;
    QJsonDocument content;
};

class JsonWebTask : public QObject {
    Q_OBJECT

  public:
    JsonWebTask(
            QNetworkAccessManager* networkAccessManager,
            QUrl baseUrl,
            JsonWebRequest request,
            QObject* parent = nullptr);
    ~JsonWebTask() override;

    // timeoutMillis <= 0: No timeout (unlimited)
    // timeoutMillis > 0: Implicitly aborted after timeout expired
    void invokeStart(
            int timeoutMillis = 0);
    void invokeAbort();

  signals:
    void networkRequestFailed(
            QString errorMessage);
    void aborted();
    void finished(
            JsonWebResponse response);

  public slots:
    void start(
            int timeoutMillis);
    void abort();

  private slots:
    void slotNetworkReplyFinished();

  protected:
    virtual void connectSlots();

    void timerEvent(QTimerEvent* event) override;

  private:
    QPointer<QNetworkAccessManager> m_networkAccessManager;
    QUrl m_baseUrl;
    JsonWebRequest m_request;
    QNetworkReply* m_networkReply;
    int m_timeoutTimerId;
    bool m_aborted;
};

} // namespace network

} // namespace mixxx

Q_DECLARE_METATYPE(mixxx::network::JsonWebResponse);
