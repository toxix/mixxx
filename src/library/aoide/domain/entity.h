#pragma once

#include "library/aoide/domain/json.h"

class AoideEntityRevision : public AoideJsonArray {
  public:
    explicit AoideEntityRevision(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }
    ~AoideEntityRevision() override = default;

    quint64 version() const;

    QDateTime timeStamp() const;
};

Q_DECLARE_METATYPE(AoideEntityRevision);

class AoideEntityHeader : public AoideJsonArray {
  public:
    explicit AoideEntityHeader(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }
    ~AoideEntityHeader() override = default;

    QString uid() const;

    AoideEntityRevision revision() const;
};

Q_DECLARE_METATYPE(AoideEntityHeader);
