#pragma once

#include "library/aoide/domain/json.h"
#include "proto/keys.pb.h"
#include "track/cue.h"
#include "util/math.h"

class AoidePositionMarker : public AoideJsonObject {
  public:
    static AoidePositionMarker fromCue(
            const Cue& cue,
            double cuePositionToMillis);

    explicit AoidePositionMarker(
            QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoidePositionMarker() override = default;

    Cue::Type type() const;
    void setType(Cue::Type type = Cue::Type::Invalid);

    double start() const;
    void setStart(double start = std::nan("start"));

    double end() const;
    void setEnd(double end = std::nan("end"));

    int number() const;
    void setNumber(int number = -1);

    QString label() const;
    void setLabel(QString label = QString());

    QColor color() const;
    void setColor(QColor color = QColor());
};

Q_DECLARE_METATYPE(AoidePositionMarker);

class AoidePositionMarkers : public AoideJsonObject {
  public:
    explicit AoidePositionMarkers(
            QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    bool locked() const;
    void setLocked(bool locked);

    QVector<AoidePositionMarker> markers() const;
    void setMarkers(QVector<AoidePositionMarker> markers);
};

Q_DECLARE_METATYPE(AoidePositionMarkers);

class AoideBeatMarker : public AoideJsonObject {
  public:
    explicit AoideBeatMarker(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideBeatMarker() override = default;

    double start() const;
    void setStart(double start = std::nan("start"));

    double end() const;
    void setEnd(double end = std::nan("end"));

    double tempoBpm(double defaultBpm) const;
    void setTempoBpm(double tempoBpm);
};

Q_DECLARE_METATYPE(AoideBeatMarker);

class AoideBeatMarkers : public AoideJsonObject {
  public:
    explicit AoideBeatMarkers(
            QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    bool locked() const;
    void setLocked(bool locked);

    QVector<AoideBeatMarker> markers() const;
    void setMarkers(QVector<AoideBeatMarker> markers);
};

Q_DECLARE_METATYPE(AoideBeatMarkers);

class AoideKeyMarker : public AoideJsonObject {
  public:
    explicit AoideKeyMarker(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideKeyMarker() override = default;

    double start() const;
    void setStart(double start = std::nan("start"));

    double end() const;
    void setEnd(double end = std::nan("end"));

    mixxx::track::io::key::ChromaticKey key() const;
    void setKey(
            mixxx::track::io::key::ChromaticKey chromaticKey =
                    mixxx::track::io::key::ChromaticKey::INVALID);
};

Q_DECLARE_METATYPE(AoideKeyMarker);

class AoideKeyMarkers : public AoideJsonObject {
  public:
    explicit AoideKeyMarkers(
            QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    bool locked() const;
    void setLocked(bool locked);

    QVector<AoideKeyMarker> markers() const;
    void setMarkers(QVector<AoideKeyMarker> markers);
};

Q_DECLARE_METATYPE(AoideKeyMarkers);
