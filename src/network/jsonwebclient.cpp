#include "network/jsonwebclient.h"

#include <QMimeDatabase>
#include <QThread>
#include <QTimerEvent>

#include <mutex> // std::once_flag

#include "util/assert.h"
#include "util/logger.h"


namespace mixxx {

namespace network {

namespace {

const Logger kLogger("mixxx::network::JsonWebClient");

std::once_flag registerMetaTypesOnceFlag;

void registerMetaTypesOnce() {
    RequestId::registerMetaType();
}

const int kDefaultTimeoutMillis = 60000; // 1 minute

const QString JSON_CONTENT_TYPE = "application/json";

const QMimeType JSON_MIME_TYPE = QMimeDatabase().mimeTypeForName(JSON_CONTENT_TYPE);

const QString TEXT_CONTENT_TYPE = "text/plain";

const QMimeType TEXT_MIME_TYPE = QMimeDatabase().mimeTypeForName(TEXT_CONTENT_TYPE);

bool readStatusCode(
        RequestId requestId,
        const QNetworkReply* reply,
        int* statusCode) {
    DEBUG_ASSERT(statusCode);
    const QVariant statusCodeAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool statusCodeValid = false;
    const int statusCodeValue = statusCodeAttr.toInt(&statusCodeValid);
    VERIFY_OR_DEBUG_ASSERT(statusCodeValid && HttpStatusCode_isValid(statusCodeValue)) {
        kLogger.warning()
                << "Invalid or missing status code attribute"
                << statusCodeAttr
                << "in reply for network request"
                << requestId;
    } else {
        *statusCode = statusCodeValue;
    }
    return statusCodeValid;
}

QMimeType readContentType(
        RequestId requestId,
        const QNetworkReply* reply) {
    DEBUG_ASSERT(reply);
    const QVariant contentTypeHeader = reply->header(QNetworkRequest::ContentTypeHeader);
    if (!contentTypeHeader.isValid() || contentTypeHeader.isNull()) {
        kLogger.warning()
                << "Missing content type header"
                << "in reply for network request"
                << requestId;
        return QMimeType();
    }
    const QString contentTypeString = contentTypeHeader.toString();
    const QString contentTypeWithoutParams = contentTypeString.left(contentTypeString.indexOf(';'));
    const QMimeType contentType = QMimeDatabase().mimeTypeForName(contentTypeWithoutParams);
    if (!contentType.isValid()) {
        kLogger.warning()
                << "Unknown content type"
                << contentTypeWithoutParams
                << "in reply for network request"
                << requestId;
    }
    return contentType;
}

bool readTextContent(
        RequestId requestId,
        QNetworkReply* reply,
        QString* textContent) {
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(textContent);
    DEBUG_ASSERT(TEXT_MIME_TYPE.isValid());
    if (readContentType(requestId, reply) == TEXT_MIME_TYPE) {
        // TODO: Verify that charset=utf-8?
        *textContent = QString::fromUtf8(reply->readAll());
        return true;
    } else {
        return false;
    }
}

bool readJsonContent(
        RequestId requestId,
        QNetworkReply* reply,
        QJsonDocument* jsonContent) {
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(jsonContent);
    DEBUG_ASSERT(JSON_MIME_TYPE.isValid());
    const auto contentType = readContentType(requestId, reply);
    if (contentType == JSON_MIME_TYPE) {
        *jsonContent = QJsonDocument::fromJson(reply->readAll());
        return true;
    } else {
        return false;
    }
}

} // anonymous namespace

JsonWebClient::JsonWebClient(
        QNetworkAccessManager* networkAccessManager,
        QObject* parent)
    : QObject(parent),
      m_networkAccessManager(networkAccessManager),
      m_requestReplyManager(this) {
    std::call_once(registerMetaTypesOnceFlag, registerMetaTypesOnce);
}

QNetworkAccessManager* JsonWebClient::accessNetwork(
        RequestId requestId) {
    DEBUG_ASSERT(thread() == QThread::currentThread());
    if (m_networkAccessManager) {
        VERIFY_OR_DEBUG_ASSERT(thread() == m_networkAccessManager->thread()) {
            kLogger.critical()
                    << "Network access from different threads is not supported:"
                    << thread()
                    << "<>"
                    << m_networkAccessManager->thread();
            emit networkRequestFailed(requestId, "Invalid thread");
            return nullptr;
        }
        if (m_networkAccessManager->networkAccessible() == QNetworkAccessManager::NotAccessible) {
            QLatin1String errorMessage("Network not accessible");
            kLogger.warning() << errorMessage;
            emit networkRequestFailed(requestId, errorMessage);
            return nullptr;
        }
    } else {
        QLatin1String errorMessage("No network access");
        kLogger.warning() << errorMessage;
        emit networkRequestFailed(requestId, errorMessage);
    }
    return m_networkAccessManager;
}

QNetworkRequest JsonWebClient::newRequest(
        const QUrl& url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, JSON_CONTENT_TYPE);
    return request;
}

void JsonWebClient::afterRequestSent(
        RequestId requestId,
        QNetworkReply* reply,
        int timeoutMillis) {
    if (timeoutMillis <= 0) {
        timeoutMillis = kDefaultTimeoutMillis;
    }
    m_requestReplyManager.afterRequestSent(
            requestId,
            reply,
            timeoutMillis);
}

void JsonWebClient::cancelRequest(
        RequestId requestId) {
    m_requestReplyManager.cancelRequest(requestId);
}

void JsonWebClient::cancelAllRequests() {
    m_requestReplyManager.cancelAllRequests();
}

std::pair<RequestId, HttpStatusCode> JsonWebClient::receiveReply(
        QNetworkReply* reply,
        QJsonDocument* jsonResponse) {
    reply->deleteLater();
    const auto requestResult = m_requestReplyManager.afterReplyReceived(reply);
    const auto requestId = requestResult.first;
    if (!requestResult.second) {
        kLogger.info()
                << "Ignoring reply for unknown or cancelled request"
                << requestId;
        return std::make_pair(requestId, kHttpStatusCodeInvalid);
    }

    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        QString errorMessage = reply->errorString();
        QString textContent;
        if (readTextContent(requestId, reply, &textContent) && !textContent.isEmpty()) {
            errorMessage += " -- ";
            errorMessage += textContent;
        }
        DEBUG_ASSERT(!errorMessage.isEmpty());
        kLogger.warning()
                << "Request"
                << requestId
                << "failed:"
                << errorMessage;
        emit networkRequestFailed(requestId, std::move(errorMessage));
        return std::make_pair(requestId, kHttpStatusCodeInvalid);
    }

    if (kLogger.debugEnabled()) {
        if (reply->url() == reply->request().url()) {
            kLogger.debug()
                    << "Received reply for request"
                    << requestId
                    << reply->url();
        } else {
            // Redirected
            kLogger.debug()
                    << "Received reply for request"
                    << requestId
                    << reply->request().url()
                    << "->"
                    << reply->url();
        }
    }

    HttpStatusCode statusCode = kHttpStatusCodeInvalid;
    if (!readStatusCode(requestId, reply, &statusCode)) {
        kLogger.warning()
                << "Failed to read HTTP status code";
    }
    if (!HttpStatusCode_isSuccess(statusCode)) {
        kLogger.warning()
                << "Request"
                << requestId
                << "failed with HTTP status code"
                << statusCode;
    }

    if (jsonResponse &&
            statusCode != kHttpStatusCodeInvalid &&
            !readJsonContent(requestId, reply, jsonResponse)) {
        kLogger.warning()
                << "Missing or invalid JSON response"
                << "in reply for request"
                << requestId;
    }

    return std::make_pair(requestId, statusCode);
}

} // namespace network

} // namespace mixxx
