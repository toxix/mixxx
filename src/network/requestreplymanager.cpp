#include "network/requestreplymanager.h"

#include <QTimerEvent>

#include "util/assert.h"
#include "util/logger.h"


namespace mixxx {

namespace network {

namespace {

const Logger kLogger("mixxx::network::RequestReplyManager");

constexpr int kInvalidTimerId = -1;

} // anonymous namespace

RequestReplyManager::RequestReplyManager(
        QObject* parent)
    : QObject(parent) {
}

RequestReplyManager::~RequestReplyManager() {
    DEBUG_ASSERT(m_pendingRequests.isEmpty());
    DEBUG_ASSERT(m_pendingReplies.isEmpty());
}

void RequestReplyManager::afterRequestSent(
        RequestId requestId,
        QNetworkReply* reply,
        int timeoutMillis) {
    DEBUG_ASSERT(requestId.isValid());
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(timeoutMillis >= 0);

    VERIFY_OR_DEBUG_ASSERT(!isRequestPending(requestId)) {
        kLogger.critical()
                << "Duplicate request identifier"
                << requestId
                << "for pending request detected"
                << "- cancelling pending request";
        cancelRequest(requestId);
    }
    DEBUG_ASSERT(!m_pendingRequests.contains(requestId));
    DEBUG_ASSERT(!m_pendingRequests.values().contains(reply));
    DEBUG_ASSERT(!m_pendingReplies.contains(reply));
    m_pendingRequests.insert(requestId, reply);

    int timerId = kInvalidTimerId;
    if (reply->isRunning() && timeoutMillis > 0) {
        if (kLogger.traceEnabled()) {
            kLogger.trace()
                    << "Starting timer with"
                    << timeoutMillis
                    << "[ms] for request"
                    << requestId;
        }
        timerId = startTimer(timeoutMillis);
        DEBUG_ASSERT(timerId != kInvalidTimerId);
        if (kLogger.traceEnabled()) {
            kLogger.trace()
                    << "Started timer"
                    << timerId
                    << "for request"
                    << requestId;
        }
    }

    auto pendingReply = PendingReply{ requestId, timerId };
    m_pendingReplies.insert(reply, pendingReply);

    if (kLogger.debugEnabled()) {
        kLogger.debug()
                << "Awaiting reply for request"
                << requestId
                << reply->request().url();
    }
}

void RequestReplyManager::cancelRequest(
        RequestId requestId) {
    QNetworkReply* reply = m_pendingRequests.take(requestId);
    if (!reply) {
        kLogger.debug()
                << "Cannot cancel unknown request"
                << requestId;
        return;
    }
    if (reply->isRunning()) {
        kLogger.info()
                << "Aborting reply of pending request"
                << requestId;
        reply->abort();
    } else {
        kLogger.debug()
                << "Reply for pending request"
                << requestId
                << "has already been finished";
    }
    DEBUG_ASSERT(!isRequestPending(requestId));
}

void RequestReplyManager::cancelAllRequests() {
    while (!m_pendingRequests.isEmpty()) {
        const auto requestId = m_pendingRequests.firstKey();
        cancelRequest(requestId);
        DEBUG_ASSERT(!m_pendingRequests.contains(requestId));
    }
}

std::pair<RequestId, bool> RequestReplyManager::afterReplyReceived(
        QNetworkReply* reply) {
    DEBUG_ASSERT(reply);
    DEBUG_ASSERT(m_pendingReplies.contains(reply));

    // Housekeeping
    const auto pendingReply = m_pendingReplies.take(reply);
    if (pendingReply.timerId != kInvalidTimerId) {
        killTimer(pendingReply.timerId);
    }

    const auto requestId = pendingReply.requestId;
    if (!m_pendingRequests.contains(requestId)) {
        // Unknown or cancelled request
        return std::make_pair(requestId, false);
    }

    // The following VERIFY_OR_DEBUG_ASSERT has a side-effect!
    VERIFY_OR_DEBUG_ASSERT(
            m_pendingRequests.take(requestId) == reply) {
        // This must never happen!!!
        kLogger.critical()
                << "Mismatching request identifier"
                << requestId
                << "for received reply detected";
        return std::make_pair(requestId, false);
    }
    DEBUG_ASSERT(!m_pendingRequests.contains(requestId));

    return std::make_pair(requestId, true);
}

RequestId RequestReplyManager::killTimerForPendingReply(int timerId) {
    DEBUG_ASSERT(timerId != kInvalidTimerId);
    killTimer(timerId); // oneshot timer
    for (auto&& pendingReply : m_pendingReplies) {
        if (pendingReply.timerId == timerId) {
            pendingReply.timerId = kInvalidTimerId;
            const auto requestId = pendingReply.requestId;
            DEBUG_ASSERT(requestId.isValid());
            return requestId;
        }
    }
    return RequestId();
}

void RequestReplyManager::timerEvent(QTimerEvent* event) {
    const auto timerId = event->timerId();
    DEBUG_ASSERT(timerId != kInvalidTimerId);
    const auto requestId = killTimerForPendingReply(timerId);
    if (!requestId.isValid()) {
        // The corresponding request may have been finished or cancelled
        // before the timeout signal is received.
        kLogger.debug()
                << "No pending request found for timer"
                << timerId
                << "after timeout";
        return;
    }
    kLogger.info()
            << "Cancelling request"
            << requestId
            << "after timeout";
    cancelRequest(requestId);
}

} // namespace network

} // namespace mixxx
