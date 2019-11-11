#include "library/aoide/domain/collection.h"

#include "util/assert.h"

QString AoideCollection::name() const {
    return m_jsonObject.value("n").toString();
}

void AoideCollection::setName(QString name) {
    putOptionalNonEmpty("n", std::move(name));
}

QString AoideCollection::description() const {
    return m_jsonObject.value("d").toString();
}

void AoideCollection::setDescription(QString description) {
    putOptionalNonEmpty("d", std::move(description));
}

AoideEntityHeader AoideCollectionEntity::header() const {
    return AoideEntityHeader(m_jsonArray.at(0).toArray());
}

AoideCollection AoideCollectionEntity::body() const {
    return AoideCollection(m_jsonArray.at(1).toObject());
}
