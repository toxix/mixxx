#pragma once

#include <QMetaType>

namespace mixxx {

namespace network {

typedef int HttpStatusCode;

const HttpStatusCode kHttpStatusCodeInvalid = -1;
const HttpStatusCode kHttpStatusCodeOk = 200;
const HttpStatusCode kHttpStatusCodeCreated = 201;
const HttpStatusCode kHttpStatusCodeAccepted = 202;
const HttpStatusCode kHttpStatusCodeNoContent = 204;

inline int HttpStatusCode_isValid(
        int statusCode) {
    return statusCode >= 100 && statusCode < 600;
}

inline int HttpStatusCode_isInformational(
        int statusCode) {
    return statusCode >= 100 && statusCode < 200;
}

inline int HttpStatusCode_isSuccess(
        int statusCode) {
    return statusCode >= 200 && statusCode < 300;
}

inline int HttpStatusCode_isRedirection(
        int statusCode) {
    return statusCode >= 300 && statusCode < 400;
}

inline int HttpStatusCode_isClientError(
        int statusCode) {
    return statusCode >= 400 && statusCode < 500;
}

inline int HttpStatusCode_isServerError(
        int statusCode) {
    return statusCode >= 400 && statusCode < 500;
}

} // namespace network

} // namespace mixxx

Q_DECLARE_METATYPE(mixxx::network::HttpStatusCode);
