#include "library/aoide/domain/entity.h"

#include "util/assert.h"

quint64 AoideEntityRevision::version() const {
    DEBUG_ASSERT(m_jsonArray.isEmpty() || m_jsonArray.size() == 2);
    if (m_jsonArray.size() == 2) {
        return m_jsonArray.at(0).toVariant().toULongLong();
    }
    return 0;
}

QDateTime AoideEntityRevision::timeStamp() const {
    DEBUG_ASSERT(m_jsonArray.isEmpty() || m_jsonArray.size() == 2);
    if (m_jsonArray.size() == 2) {
        return importDateTimeTicks(m_jsonArray.at(1));
    }
    return QDateTime();
}

QString AoideEntityHeader::uid() const {
    if (m_jsonArray.size() == 2) {
        return m_jsonArray.at(0).toString();
    }
    return QString();
}

AoideEntityRevision AoideEntityHeader::revision() const {
    if (m_jsonArray.size() == 2) {
        return AoideEntityRevision(m_jsonArray.at(1).toArray());
    }
    return AoideEntityRevision();
}
