#pragma once

#include <QByteArray>
#include <QImage>

namespace mixxx {

QString encodeFingerprint(const QByteArray& bytes);
QByteArray decodeFingerprint(const QString& fingerprint);

QByteArray hashImage(const QImage& image);

quint64 cacheKey(const QByteArray& bytes);

}
