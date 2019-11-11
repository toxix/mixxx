#pragma once

#include "library/aoide/domain/entity.h"

class AoidePlaylistEntry : public AoideJsonObject {
  public:
    static AoidePlaylistEntry newSeparator();
    static AoidePlaylistEntry newTrack(QString trackUid);

    explicit AoidePlaylistEntry(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoidePlaylistEntry() override = default;

    QDateTime since() const;
    void setSince(QDateTime since = QDateTime());

    QString comment() const;
    void setComment(QString comment = QString());

    QString trackUid() const;
};

Q_DECLARE_METATYPE(AoidePlaylistEntry);

class AoidePlaylist : public AoideJsonObject {
  public:
    explicit AoidePlaylist(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoidePlaylist() override = default;

    QString name() const;
    void setName(QString name = QString());

    QString description() const;
    void setDescription(QString description = QString());

    QString type() const;
    void setType(QString type = QString());

    QColor color() const;
    void setColor(QColor color = QColor());

    QJsonArray entries() const;
    void setEntries(QJsonArray entries);
};

Q_DECLARE_METATYPE(AoidePlaylist);

class AoidePlaylistEntity : public AoideJsonArray {
  public:
    explicit AoidePlaylistEntity(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }
    ~AoidePlaylistEntity() override = default;

    AoideEntityHeader header() const;

    AoidePlaylist body() const;
};

Q_DECLARE_METATYPE(AoidePlaylistEntity);

class AoidePlaylistBriefEntries : public AoideJsonObject {
  public:
    explicit AoidePlaylistBriefEntries(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoidePlaylistBriefEntries() override = default;

    quint64 tracksCount() const;

    quint64 entriesCount() const;

    QDateTime entriesSinceMin() const;

    QDateTime entriesSinceMax() const;
};

Q_DECLARE_METATYPE(AoidePlaylistBriefEntries);

class AoidePlaylistBrief : public AoideJsonObject {
  public:
    explicit AoidePlaylistBrief(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoidePlaylistBrief() override = default;

    QString name() const;

    QString description() const;

    QString type() const;

    QColor color() const;

    AoidePlaylistBriefEntries entries() const;
};

Q_DECLARE_METATYPE(AoidePlaylistBrief);

class AoidePlaylistBriefEntity : public AoideJsonArray {
  public:
    explicit AoidePlaylistBriefEntity(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }
    ~AoidePlaylistBriefEntity() override = default;

    AoideEntityHeader header() const;

    AoidePlaylistBrief body() const;
};

Q_DECLARE_METATYPE(AoidePlaylistBriefEntity);
