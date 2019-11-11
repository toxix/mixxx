#include "library/aoide/domain/json.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QTimeZone>
#include <QVariant>

#include "util/assert.h"
#include "util/logger.h"
#include "util/math.h"

namespace {

const mixxx::Logger kLogger("aoide JSON");

const QRegularExpression kRegexpWhitespace("\\s+");

} // anonymous namespace

void AoideJsonObject::putOptional(QString key, const QString& value) {
    if (value.isNull()) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), value);
    }
}

void AoideJsonObject::putOptional(QString key, double value) {
    if (util_isnan(value)) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), value);
    }
}

void AoideJsonObject::putOptionalNonEmpty(QString key, QJsonValue value) {
    if (value.isUndefined() || value.isNull() ||
            (value.isObject() && value.toObject().isEmpty()) ||
            (value.isArray() && value.toArray().isEmpty())) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), std::move(value));
    }
}

void AoideJsonObject::putOptionalNonEmpty(QString key, QJsonArray array) {
    if (array.isEmpty()) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), std::move(array));
    }
}

void AoideJsonObject::putOptionalNonEmpty(QString key, QJsonObject object) {
    if (object.isEmpty()) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), std::move(object));
    }
}

void AoideJsonObject::putOptionalNonEmpty(QString key, const QString& value) {
    if (value.isEmpty()) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), std::move(value));
    }
}

void AoideJsonObject::putOptionalNonEmpty(QString key, const QVariant& value) {
    auto jsonValue = QJsonValue::fromVariant(value);
    DEBUG_ASSERT(!jsonValue.isUndefined());
    if (jsonValue.isNull() ||
            (jsonValue.isString() && jsonValue.toString().isEmpty())) {
        m_jsonObject.remove(key);
    } else {
        m_jsonObject.insert(std::move(key), std::move(jsonValue));
    }
}

namespace {

QString formatDateTime(QDateTime dt) {
    if (!dt.timeZone().isValid() || dt.timeSpec() == Qt::LocalTime) {
        // Assume UTC if no time zone is specified or local time
        dt.setTimeSpec(Qt::UTC);
    }
    if (dt.time().msec() == 0) {
        return dt.toString(Qt::ISODate);
    } else {
        return dt.toString(Qt::ISODateWithMs);
    }
}

} // anonymous namespace

//static
QVariant AoideJsonBase::exportDateTimeOrYear(const QString& value) {
    // To upper: 't' -> 'T', 'z' -> 'Z'
    const auto trimmed = value.toUpper().trimmed();
    auto compact = trimmed;
    compact
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
            .remove(kRegexpWhitespace);
#else
            .replace(kRegexpWhitespace, QString());
#endif
    if (compact.isEmpty()) {
        return QVariant(); // null
    }
    if (compact.contains('T')) {
        // Full ISO 8601 / RFC 3339 time stamp
        auto datetime = QDateTime::fromString(compact, Qt::ISODateWithMs);
        if (datetime.isValid()) {
            return formatDateTime(datetime);
        }
        // Try to append missing seconds
        datetime = QDateTime::fromString(compact + ":00", Qt::ISODate);
        if (datetime.isValid()) {
            return formatDateTime(datetime);
        }
    } else if (trimmed.contains(':')) {
        auto datetime = QDateTime::fromString(trimmed, Qt::RFC2822Date);
        if (datetime.isValid()) {
            return formatDateTime(datetime);
        }
        // Non-standard date/time time stamp
        datetime = QDateTime::fromString(trimmed, "yyyy-M-d h:m:s");
        if (datetime.isValid()) {
            return formatDateTime(datetime);
        }
        // Try to parse without seconds
        datetime = QDateTime::fromString(trimmed, "yyyy-M-d h:m");
        if (datetime.isValid()) {
            return formatDateTime(datetime);
        }
    } else {
        // Simple date with both month and day optional
        // Try to parse year + month + day
        auto date = QDate::fromString(compact, Qt::ISODate);
        if (date.isValid()) {
            return QVariant(date.year() * 10000 + date.month() * 100 + date.day());
        }
        date = QDate::fromString(trimmed, Qt::RFC2822Date);
        if (date.isValid()) {
            return QVariant(date.year() * 10000 + date.month() * 100 + date.day());
        }
        // Try to parse simple date: year + month + day
        date = QDate::fromString(compact, "yyyy-M-d");
        if (date.isValid()) {
            return QVariant(date.year() * 10000 + date.month() * 100 + date.day());
        }
        // Try to parse incomplete date: year + month (without day)
        date = QDate::fromString(compact, "yyyy-M");
        if (date.isValid()) {
            return QVariant(date.year() * 10000 + date.month() * 100 /*day = 0*/);
        }
        // Try to parse a single row of digits
        date = QDate::fromString(compact, "yyyyMMdd");
        if (date.isValid()) {
            return QVariant(date.year() * 10000 + date.month() * 100 + date.day());
        }
        date = QDate::fromString(compact, "yyyy");
        if (date.isValid()) {
            return QVariant(date.year() * 10000 /*month = 0, day = 0*/);
        }
    }
    // TODO: Add any missing cases that are reported. Don't forget
    // to extend the corresponding unit test to prevent regressions!
    kLogger.warning()
            << "Failed to parse date/time from string"
            << value;
    return QVariant();
}

//static
QString AoideJsonBase::importDateTimeOrYear(const QJsonValue& value) {
    if (value.isUndefined() || value.isNull()) {
        return QString();
    }
    if (value.isString()) {
        return value.toString();
    }
    auto ymd = value.toInt(0);
    if (ymd < 10000) {
        return QString();
    }
    int d = ymd % 100;
    ymd /= 100;
    int m = ymd % 100;
    ymd /= 100;
    auto year = QString("%1").arg(ymd, 4, 10, QLatin1Char('0'));
    QString month;
    if (m > 0 || d > 0) {
        month = QString("-%1").arg(m, 2, 10, QLatin1Char('0'));
    }
    QString day;
    if (d > 0) {
        day = QString("-%1").arg(d, 2, 10, QLatin1Char('0'));
    }
    return QString("%1%2%3").arg(year, month, day);
}

namespace {

QDateTime normalizeDateTimeBeforeExport(QDateTime value) {
    if (value.isNull()) {
        return value;
    }
    DEBUG_ASSERT(value.isValid());
    if (!value.timeZone().isValid() || (value.timeSpec() == Qt::LocalTime)) {
        // No time zone -> assume UTC
        value.setTimeZone(QTimeZone::utc());
    }
    if ((value.timeSpec() == Qt::LocalTime) || (value.timeSpec() == Qt::TimeZone)) {
        // Convert time zone to offset from UTC
        value = value.toTimeSpec(Qt::OffsetFromUTC);
    }
    if (value.timeZone() == QTimeZone::utc()) {
        // Enforce suffix 'Z' when formatting
        // NOTE(uklotzde, 2018-05-11): Don't remove this code!! Otherwise
        // the suffix might be omitted and tests will fail intentionally.
        value.setTimeSpec(Qt::UTC);
    }
    DEBUG_ASSERT(value.timeZone().isValid());
    DEBUG_ASSERT((value.timeSpec() == Qt::UTC) || (value.timeSpec() == Qt::OffsetFromUTC));
    return value;
}

} // anonymous namespace

/*static*/ QString AoideJsonBase::exportDateTime(QDateTime value) {
    value = normalizeDateTimeBeforeExport(value);
    DEBUG_ASSERT(value.timeZone().isValid());
    DEBUG_ASSERT((value.timeSpec() == Qt::UTC) || (value.timeSpec() == Qt::OffsetFromUTC));
    if ((value.toMSecsSinceEpoch() % 1000) == 0) {
        // Omit milliseconds if zero
        return value.toString(Qt::ISODate);
    } else {
        // Use full precision
        return value.toString(Qt::ISODateWithMs);
    }
}

/*static*/ QVariant AoideJsonBase::exportDateTimeTicks(QDateTime value) {
    value = normalizeDateTimeBeforeExport(value);
    DEBUG_ASSERT(value.timeZone().isValid());
    DEBUG_ASSERT((value.timeSpec() == Qt::UTC) || (value.timeSpec() == Qt::OffsetFromUTC));
    quint64 msecs = value.toMSecsSinceEpoch();
    return QVariant(msecs * 1000);
}

/*static*/ QDateTime AoideJsonBase::importDateTimeTicks(QJsonValue value) {
    const auto variant = value.toVariant();
    if (variant.isValid()) {
        bool ok;
        const auto micros = variant.toULongLong(&ok);
        if (ok) {
            return QDateTime::fromMSecsSinceEpoch(micros / 1000);
        }
    }
    DEBUG_ASSERT(!"Invalid or missing time stamp");
    return QDateTime();
}

/*static*/ QString AoideJsonBase::formatUuid(const QUuid& uuid) {
    if (uuid.isNull()) {
        return QString(); // null
    } else {
        QString formatted = uuid.toString();
        if (formatted.size() == 38) {
            // We need to strip off the heading/trailing curly braces after formatting
            formatted = formatted.mid(1, 36);
        }
        DEBUG_ASSERT(formatted.size() == 36);
        return formatted;
    }
}

//static
QString AoideJsonBase::formatColor(const QColor& color) {
    if (color.isValid()) {
        return color.name(QColor::HexRgb);
    } else {
        return QString();
    }
}

//static
QStringList AoideJsonBase::toStringList(QJsonArray jsonArray) {
    QStringList result;
    result.reserve(jsonArray.size());
    for (auto&& jsonValue : qAsConst(jsonArray)) {
        result += jsonValue.toString();
    }
    return result;
}
