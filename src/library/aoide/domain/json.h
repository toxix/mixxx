#pragma once

#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QUuid>

///////////////////////////////////////////////////////////////////////////////
// Thin and efficient wrappers around QJsonObject with some accessors and
// only limited editing functionality.
//
// Signal types should to be placed in the global namespace! Handling of
// meta types in namespaces is hardly documented and may not work as expected.
///////////////////////////////////////////////////////////////////////////////

class AoideJsonBase {
  public:
    virtual ~AoideJsonBase() = default;

    // RFC 3339
    static QString exportDateTime(QDateTime value);
    static QVariant exportDateTimeOrYear(const QString& value);
    static QString importDateTimeOrYear(const QJsonValue& value);

    // Ticks (= microseconds) since epoch (UTC)
    static QVariant exportDateTimeTicks(QDateTime value);
    static QDateTime importDateTimeTicks(QJsonValue value);

    static QString formatUuid(const QUuid& uuid);

    static QString formatColor(const QColor& color);

    static QStringList toStringList(QJsonArray jsonArray);
};

Q_DECLARE_METATYPE(AoideJsonBase);

class AoideJsonObject : public AoideJsonBase {
  public:
    explicit AoideJsonObject(QJsonObject jsonObject = QJsonObject())
            : m_jsonObject(std::move(jsonObject)) {
    }
    ~AoideJsonObject() override = default;

    bool isEmpty() const {
        return m_jsonObject.isEmpty();
    }

    operator const QJsonObject&() const {
        return m_jsonObject;
    }

    virtual QJsonValue intoJsonValue() {
        return std::move(m_jsonObject);
    }

  protected:
    void putOptional(QString key, const QString& value);
    void putOptional(QString key, double value);
    void putOptionalNonEmpty(QString key, const QString& value);
    void putOptionalNonEmpty(QString key, QJsonObject object);
    void putOptionalNonEmpty(QString key, QJsonArray array);
    void putOptionalNonEmpty(QString key, QJsonValue value);
    void putOptionalNonEmpty(QString key, const QVariant& value);

    QJsonObject m_jsonObject;
};

Q_DECLARE_METATYPE(AoideJsonObject);

class AoideJsonArray : public AoideJsonBase {
  public:
    explicit AoideJsonArray(QJsonArray jsonArray = QJsonArray())
            : m_jsonArray(std::move(jsonArray)) {
    }
    ~AoideJsonArray() override = default;

    bool isEmpty() const {
        return m_jsonArray.isEmpty();
    }

    void shrink(int shrinkedSize) {
        while (m_jsonArray.size() > shrinkedSize) {
            m_jsonArray.removeLast();
        }
    }

    operator const QJsonArray&() const {
        return m_jsonArray;
    }

    virtual QJsonValue intoJsonValue() {
        return std::move(m_jsonArray);
    }

  protected:
    QJsonArray m_jsonArray;
};

Q_DECLARE_METATYPE(AoideJsonArray);
