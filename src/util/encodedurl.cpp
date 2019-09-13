#include "util/encodedurl.h"

//static
EncodedUrl EncodedUrl::fromUrl(const QUrl& url) {
    return fromUrlEncoded(url.toEncoded(
            QUrl::StripTrailingSlash |
            QUrl::NormalizePathSegments));
}

QUrl EncodedUrl::toUrl() const {
    return QUrl::fromEncoded(m_urlEncoded, QUrl::StrictMode);
}
