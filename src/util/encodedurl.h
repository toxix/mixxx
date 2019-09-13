#pragma once

#include <QByteArray>
#include <QUrl>
#include <QtDebug>

// A URL-encoded QUrl
class EncodedUrl final {
  public:
    static EncodedUrl fromUrlEncoded(QByteArray urlEncoded) {
        return EncodedUrl(std::move(urlEncoded));
    }
    static EncodedUrl fromUrl(const QUrl& url);

    EncodedUrl() = default;
    EncodedUrl(const EncodedUrl&) = default;
    EncodedUrl(EncodedUrl&&) = default;

    bool isValid() const {
        return !m_urlEncoded.isEmpty();
    }

    QString toString() const {
        return m_urlEncoded;
    }

    QUrl toUrl() const;

  private:
    explicit EncodedUrl(QByteArray urlEncoded)
            : m_urlEncoded(std::move(urlEncoded)) {
    }

    QByteArray m_urlEncoded;
};

inline QDebug
operator<<(QDebug debug, const EncodedUrl& trackUri) {
    return debug << trackUri.toString();
}
