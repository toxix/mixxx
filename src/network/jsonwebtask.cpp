#include "network/jsonwebtask.h"

#include <QMimeDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimerEvent>
#include <mutex> // std::once_flag

#include "util/assert.h"
#include "util/logger.h"

namespace mixxx {

namespace network {

namespace {

const Logger kLogger("mixxx::network::JsonWebTask");

constexpr int kInvalidTimerId = -1;

std::once_flag registerMetaTypesOnceFlag;

void registerMetaTypesOnce() {
    JsonWebResponse::registerMetaType();
}

const QString JSON_CONTENT_TYPE = "application/json";

const QMimeType JSON_MIME_TYPE = QMimeDatabase().mimeTypeForName(JSON_CONTENT_TYPE);

const QString TEXT_CONTENT_TYPE = "text/plain";

const QMimeType TEXT_MIME_TYPE = QMimeDatabase().mimeTypeForName(TEXT_CONTENT_TYPE);

bool readStatusCode(
        const QNetworkReply* reply,
        int* statusCode) {
    DEBUG_ASSERT(statusCode);
    const QVariant statusCodeAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool statusCodeValid = false;
    const int statusCodeValue = statusCodeAttr.toInt(&statusCodeValid);
    VERIFY_OR_DEBUG_ASSERT(statusCodeValid && HttpStatusCode_isValid(statusCodeValue)) {
        kLogger.warning()
                << "Invalid or missing status code attribute"
                << statusCodeAttr;
    }
    else {
        *statusCode = statusCodeValue;
    }
    return statusCodeValid;
}

QMimeType readContentType(
        const QNetworkReply* reply) {
    DEBUG_ASSERT(reply);
    const QVariant contentTypeHeader = reply->header(QNetworkRequest::ContentTypeHeader);
    if (!contentTypeHeader.isValid() || contentTypeHeader.isNull()) {
        kLogger.warning()
                << "Missing content type header";
        return QMimeType();
    }
    const QString contentTypeString = contentTypeHeader.toString();
    const QString contentTypeWithoutParams = contentTypeString.left(contentTypeString.indexOf(';'));
    const QMimeType contentType = QMimeDatabase().mimeTypeForName(contentTypeWithoutParams);
    if (!contentType.isValid()) {
        kLogger.warning()
                << "Unknown content type"
                << contentTypeWithoutParams;
    }
    return contentType;
}

bool readTextContent(
        QNetworkReply* reply,
        QString* textContent) {
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(textContent);
    DEBUG_ASSERT(TEXT_MIME_TYPE.isValid());
    if (readContentType(reply) == TEXT_MIME_TYPE) {
        // TODO: Verify that charset=utf-8?
        *textContent = QString::fromUtf8(reply->readAll());
        return true;
    } else {
        return false;
    }
}

bool readJsonContent(
        QNetworkReply* reply,
        QJsonDocument* jsonContent) {
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(jsonContent);
    DEBUG_ASSERT(JSON_MIME_TYPE.isValid());
    const auto contentType = readContentType(reply);
    if (contentType == JSON_MIME_TYPE) {
        *jsonContent = QJsonDocument::fromJson(reply->readAll());
        return true;
    } else {
        return false;
    }
}

// TODO: Allow to customize headers and attributes?
QNetworkRequest newRequest(
        const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, JSON_CONTENT_TYPE);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    return request;
}

} // anonymous namespace

/*static*/ void JsonWebResponse::registerMetaType() {
    qRegisterMetaType<JsonWebResponse>("mixxx::network::JsonWebResponse");
}

JsonWebTask::JsonWebTask(
        QNetworkAccessManager* networkAccessManager,
        QUrl baseUrl,
        JsonWebRequest request,
        QObject* parent)
        : QObject(parent),
          m_networkAccessManager(networkAccessManager),
          m_baseUrl(std::move(baseUrl)),
          m_request(std::move(request)),
          m_networkReply(nullptr),
          m_timeoutTimerId(kInvalidTimerId),
          m_aborted(false) {
    std::call_once(registerMetaTypesOnceFlag, registerMetaTypesOnce);
    DEBUG_ASSERT(m_networkAccessManager);
    DEBUG_ASSERT(!m_baseUrl.isEmpty());
}

JsonWebTask::~JsonWebTask() {
    if (m_networkReply) {
        m_networkReply->deleteLater();
    }
}

void JsonWebTask::connectSlots() {
    connect(m_networkReply,
            &QNetworkReply::finished,
            this,
            &JsonWebTask::slotNetworkReplyFinished);
}

void JsonWebTask::invokeStart(int timeoutMillis) {
    QMetaObject::invokeMethod(
            this,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            "start",
            Qt::AutoConnection,
            Q_ARG(int, timeoutMillis)
#else
            [this, timeoutMillis] {
                this->start(timeoutMillis);
            }
#endif
    );
}

void JsonWebTask::invokeAbort() {
    QMetaObject::invokeMethod(
            this,
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
            "abort"
#else
            [this] {
                this->abort();
            }
#endif
    );
}

void JsonWebTask::start(int timeoutMillis) {
    VERIFY_OR_DEBUG_ASSERT(m_networkAccessManager) {
        kLogger.warning()
                << "No network access";
        return;
    }
    VERIFY_OR_DEBUG_ASSERT(!m_networkReply) {
        kLogger.warning()
                << "Task has already been started";
        return;
    }

    DEBUG_ASSERT(m_baseUrl.isValid());
    QUrl url = std::move(m_baseUrl);
    url.setPath(m_request.path);
    if (!m_request.query.isEmpty()) {
        url.setQuery(m_request.query);
    }
    DEBUG_ASSERT(url.isValid());

    switch (m_request.method) {
    case HttpRequestMethod::Get: {
        DEBUG_ASSERT(m_request.content.isEmpty());
        kLogger.debug()
                << "GET"
                << url;
        m_networkReply = m_networkAccessManager->get(
                QNetworkRequest(url));
        break;
    }
    case HttpRequestMethod::Put: {
        auto content = m_request.content.toJson(QJsonDocument::Compact);
        kLogger.debug()
                << "PUT"
                << url
                << content;
        m_networkReply = m_networkAccessManager->put(
                newRequest(url),
                std::move(content));
        break;
    }
    case HttpRequestMethod::Post: {
        auto content = m_request.content.toJson(QJsonDocument::Compact);
        kLogger.debug()
                << "POST"
                << url
                << content;
        m_networkReply = m_networkAccessManager->post(
                newRequest(url),
                std::move(content));
        break;
    }
    case HttpRequestMethod::Delete: {
        DEBUG_ASSERT(m_request.content.isEmpty());
        kLogger.debug()
                << "DELETE"
                << url;
        m_networkReply = m_networkAccessManager->deleteResource(
                QNetworkRequest(url));
        break;
    }
    }
    connectSlots();
    m_networkAccessManager = nullptr;

    if (timeoutMillis > 0) {
        m_timeoutTimerId = startTimer(timeoutMillis);
        DEBUG_ASSERT(m_timeoutTimerId != kInvalidTimerId);
    }
}

void JsonWebTask::abort() {
    if (!m_networkReply) {
        return;
    }
    m_aborted = true;
    m_networkReply->abort();
}

void JsonWebTask::timerEvent(QTimerEvent* event) {
    const auto timerId = event->timerId();
    DEBUG_ASSERT(timerId != kInvalidTimerId);
    if (timerId != m_timeoutTimerId) {
        // ignore
    }
    killTimer(timerId);
    m_timeoutTimerId = kInvalidTimerId;
    abort();
}

void JsonWebTask::slotNetworkReplyFinished() {
    auto* const networkReply = qobject_cast<QNetworkReply*>(sender());
    DEBUG_ASSERT(networkReply == m_networkReply);
    networkReply->disconnect();
    networkReply->deleteLater();
    VERIFY_OR_DEBUG_ASSERT(m_networkReply) {
        return;
    }
    m_networkReply = nullptr;

    if (m_aborted) {
        emit aborted();
        return;
    }

    if (networkReply->error() != QNetworkReply::NetworkError::NoError) {
        QString errorMessage = networkReply->errorString();
        QString textContent;
        if (readTextContent(networkReply, &textContent) && !textContent.isEmpty()) {
            errorMessage += " -- ";
            errorMessage += textContent;
        }
        DEBUG_ASSERT(!errorMessage.isEmpty());
        emit networkRequestFailed(std::move(errorMessage));
        return;
    }

    if (kLogger.debugEnabled()) {
        if (networkReply->url() == networkReply->request().url()) {
            kLogger.debug()
                    << "Received reply for request"
                    << networkReply->url();
        } else {
            // Redirected
            kLogger.debug()
                    << "Received reply for redirected request"
                    << networkReply->request().url()
                    << "->"
                    << networkReply->url();
        }
    }

    HttpStatusCode statusCode = kHttpStatusCodeInvalid;
    VERIFY_OR_DEBUG_ASSERT(readStatusCode(networkReply, &statusCode)) {
        kLogger.warning()
                << "Failed to read HTTP status code";
    }
    if (!HttpStatusCode_isSuccess(statusCode)) {
        kLogger.warning()
                << "Reply"
                << networkReply->url()
                << "failed with HTTP status code"
                << statusCode;
    }

    QJsonDocument content;
    if (statusCode != kHttpStatusCodeInvalid &&
            !readJsonContent(networkReply, &content)) {
        kLogger.warning()
                << "Reply"
                << networkReply->url()
                << "has no JSON content";
    }

    emit finished(JsonWebResponse{
            statusCode,
            std::move(content)});
}

} // namespace network

} // namespace mixxx
