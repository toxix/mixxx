#pragma once

#include "library/aoide/domain/entity.h"

class AoideCollection : public AoideJsonObject {
  public:
    explicit AoideCollection(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideCollection() override = default;

    QString name() const;
    void setName(QString name = QString());

    QString description() const;
    void setDescription(QString description = QString());
};

Q_DECLARE_METATYPE(AoideCollection);

class AoideCollectionEntity : public AoideJsonArray {
  public:
    explicit AoideCollectionEntity(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }
    ~AoideCollectionEntity() override = default;

    AoideEntityHeader header() const;

    AoideCollection body() const;
};

Q_DECLARE_METATYPE(AoideCollectionEntity);
