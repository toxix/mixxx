#include "library/aoide/domain/playlist.h"

#include "util/assert.h"

QDateTime AoidePlaylistEntry::since() const {
    return importDateTimeTicks(m_jsonObject.value("s"));
}

void AoidePlaylistEntry::setSince(QDateTime since) {
    putOptionalNonEmpty("s", exportDateTimeTicks(std::move(since)));
}

QString AoidePlaylistEntry::comment() const {
    return m_jsonObject.value("m").toString();
}

void AoidePlaylistEntry::setComment(QString comment) {
    putOptionalNonEmpty("m", std::move(comment));
}

QString AoidePlaylistEntry::trackUid() const {
    return m_jsonObject.value("i").toObject().value("t").toString();
}

/*static*/ AoidePlaylistEntry AoidePlaylistEntry::newSeparator() {
    AoidePlaylistEntry entry;
    entry.m_jsonObject.insert("i", "s");
    return entry;
}

/*static*/ AoidePlaylistEntry AoidePlaylistEntry::newTrack(QString trackUid) {
    DEBUG_ASSERT(!trackUid.isEmpty());
    QJsonObject itemObject;
    itemObject.insert("t", std::move(trackUid));
    AoidePlaylistEntry entry;
    entry.m_jsonObject.insert("i", std::move(itemObject));
    return entry;
}

QString AoidePlaylist::name() const {
    return m_jsonObject.value("n").toString();
}

void AoidePlaylist::setName(QString name) {
    putOptionalNonEmpty("n", std::move(name));
}

QString AoidePlaylist::description() const {
    return m_jsonObject.value("d").toString();
}

void AoidePlaylist::setDescription(QString description) {
    putOptionalNonEmpty("d", std::move(description));
}

QString AoidePlaylist::type() const {
    return m_jsonObject.value("p").toString();
}

void AoidePlaylist::setType(QString type) {
    putOptionalNonEmpty("p", std::move(type));
}

QColor AoidePlaylist::color() const {
    return QColor(m_jsonObject.value("c").toString());
}

void AoidePlaylist::setColor(QColor color) {
    putOptionalNonEmpty("c", formatColor(color));
}

QJsonArray AoidePlaylist::entries() const {
    return m_jsonObject.value("e").toArray();
}

void AoidePlaylist::setEntries(QJsonArray entries) {
    // Entries array is mandatory!
    m_jsonObject.insert("e", std::move(entries));
}

AoideEntityHeader AoidePlaylistEntity::header() const {
    return AoideEntityHeader(m_jsonArray.at(0).toArray());
}

AoidePlaylist AoidePlaylistEntity::body() const {
    return AoidePlaylist(m_jsonArray.at(1).toObject());
}

quint64 AoidePlaylistBriefEntries::tracksCount() const {
    return m_jsonObject.value("t").toObject().value("n").toVariant().toULongLong();
}

quint64 AoidePlaylistBriefEntries::entriesCount() const {
    return m_jsonObject.value("e").toObject().value("n").toVariant().toULongLong();
}

QDateTime AoidePlaylistBriefEntries::entriesSinceMin() const {
    auto minmaxArray = m_jsonObject.value("e").toObject().value("s").toArray();
    if (minmaxArray.size() == 2) {
        return importDateTimeTicks(minmaxArray.at(0));
    }
    return QDateTime();
}

QDateTime AoidePlaylistBriefEntries::entriesSinceMax() const {
    auto minmaxArray = m_jsonObject.value("e").toObject().value("s").toArray();
    if (minmaxArray.size() == 2) {
        return importDateTimeTicks(minmaxArray.at(1));
    }
    return QDateTime();
}

QString AoidePlaylistBrief::name() const {
    return m_jsonObject.value("n").toString();
}

QString AoidePlaylistBrief::description() const {
    return m_jsonObject.value("d").toString();
}

QString AoidePlaylistBrief::type() const {
    return m_jsonObject.value("p").toString();
}

QColor AoidePlaylistBrief::color() const {
    return QColor(m_jsonObject.value("c").toString());
}

AoidePlaylistBriefEntries AoidePlaylistBrief::entries() const {
    return AoidePlaylistBriefEntries(m_jsonObject.value("e").toObject());
}

AoideEntityHeader AoidePlaylistBriefEntity::header() const {
    return AoideEntityHeader(m_jsonArray.at(0).toArray());
}

AoidePlaylistBrief AoidePlaylistBriefEntity::body() const {
    return AoidePlaylistBrief(m_jsonArray.at(1).toObject());
}
