#include "util/fingerprint.h"

#include <QCryptographicHash>

namespace mixxx {

namespace {

const QByteArray::Base64Options kFingerprintBase64Options =
        QByteArray::Base64Encoding | QByteArray::OmitTrailingEquals;

}

QString encodeFingerprint(const QByteArray& bytes) {
    return bytes.toBase64(kFingerprintBase64Options);
}

QByteArray decodeFingerprint(const QString& fingerprint) {
    return QByteArray::fromBase64(fingerprint.toLatin1(), kFingerprintBase64Options);
}

QByteArray hashImage(const QImage& image) {
    if (image.isNull()) {
        return QByteArray();
    }
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(
            reinterpret_cast<const char*>(image.constBits()),
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            image.sizeInBytes()
#else
            image.byteCount()
#endif
            );
    return hasher.result();
}

quint64 cacheKey(const QByteArray& bytes) {
    quint64 key = 0;
    for (int i = 0; i < bytes.size(); ++i) {
        quint64 val = bytes[i];
        key ^= val << (i % 8) * 8;
    }
    return key;
}

}
