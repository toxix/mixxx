#pragma once

#include <QMimeType>

#include "library/aoide/domain/entity.h"
#include "library/aoide/domain/marker.h"
#include "library/aoide/domain/tag.h"
#include "library/starrating.h"
#include "track/replaygain.h"
#include "util/encodedurl.h"

namespace mixxx {

class TrackInfo;

}

class AoideAudioEncoder : public AoideJsonObject {
  public:
    explicit AoideAudioEncoder(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideAudioEncoder() override = default;

    QString name() const;
    void setName(QString name = QString());

    QString settings() const;
    void setSettings(QString settings = QString());
};

Q_DECLARE_METATYPE(AoideAudioEncoder);

class AoideAudioContent : public AoideJsonObject {
  public:
    explicit AoideAudioContent(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    double durationMs(double defaultMs = 0) const;
    void setDurationMs(double durationMs);

    int channelCount(int defaultCount = 0) const;
    void setChannelCount(int channelCount);

    int sampleRateHz(int defaultHz = 0) const;
    void setSampleRateHz(int sampleRateHz);

    int bitRateBps(int defaultBps = 0) const;
    void setBitRateBps(int bitRateBps);

    double loudnessLufs() const;
    void setLoudnessLufs(double loudness);
    void resetLoudnessLufs();

    mixxx::ReplayGain replayGain() const;
    void setReplayGain(mixxx::ReplayGain replayGain = mixxx::ReplayGain());

    AoideAudioEncoder encoder() const;
    void setEncoder(AoideAudioEncoder encoder);
};

Q_DECLARE_METATYPE(AoideAudioContent);

class AoideArtwork : public AoideJsonObject {
  public:
    explicit AoideArtwork(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideArtwork() override = default;

    QSize size() const;
    void setSize(const QSize& uri);

    QString fingerprint() const;
    void setFingerprint(QString fingerprint);

    EncodedUrl uri() const;
    void setUri(const EncodedUrl& uri);

    QColor backgroundColor() const;
    void setBackgroundColor(QColor backgroundColor);

    static QColor imageBackgroundColor(const QImage& image);
};

Q_DECLARE_METATYPE(AoideArtwork);

class AoideMediaSource : public AoideJsonObject {
  public:
    explicit AoideMediaSource(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
    ~AoideMediaSource() override = default;

    EncodedUrl uri() const;
    void setUri(const EncodedUrl& uri);

    QString contentTypeName() const;
    void setContentType(QMimeType contentType);

    AoideAudioContent audioContent() const;
    void setAudioContent(AoideAudioContent audioContent = AoideAudioContent());

    AoideArtwork artwork() const;
    void setArtwork(AoideArtwork artwork = AoideArtwork());
};

Q_DECLARE_METATYPE(AoideMediaSource);

class AoideTitle : public AoideJsonArray {
  public:
    explicit AoideTitle(QJsonValue jsonValue = QJsonValue());

    QString name() const;
    void setName(QString name);

    static const int kLevelMain; // default if none specified
    static const int kLevelSub;
    static const int kLevelWork;
    static const int kLevelMovement;

    int level() const;
    void setLevel(int level);

    QJsonValue intoJsonValue() override;
};

Q_DECLARE_METATYPE(AoideTitle);

typedef QVector<AoideTitle> AoideTitleVector;

Q_DECLARE_METATYPE(AoideTitleVector);

class AoideActor : public AoideJsonArray {
  public:
    explicit AoideActor(QJsonValue jsonValue = QJsonValue());

    QString name() const;
    void setName(QString name);

    static const int kRoleArtist; // default if none specified
    static const int kRoleComposer;
    static const int kRoleConductor;
    static const int kRoleLyricist;
    static const int kRoleRemixer;

    int role() const;
    void setRole(int role);

    static const int kPrecedenceSummary; // default if none specified
    static const int kPrecedencePrimary;
    static const int kPrecedenceSecondary;

    int precedence() const;
    void setPrecedence(int precedence);

    QJsonValue intoJsonValue() override;
};

Q_DECLARE_METATYPE(AoideActor);

typedef QVector<AoideActor> AoideActorVector;

Q_DECLARE_METATYPE(AoideActorVector);

class AoideTrackOrAlbumOrRelease : public AoideJsonObject {
  public:
    explicit AoideTrackOrAlbumOrRelease(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }
};

class AoideTrackOrAlbum : public AoideTrackOrAlbumOrRelease {
  public:
    explicit AoideTrackOrAlbum(QJsonObject jsonObject = QJsonObject())
            : AoideTrackOrAlbumOrRelease(std::move(jsonObject)) {
    }

    AoideTitleVector titles(int level = AoideTitle::kLevelMain) const;
    AoideTitleVector allTitles() const;
    AoideTitleVector removeTitles(int level);
    AoideTitleVector clearTitles();
    void addTitles(AoideTitleVector titles);

    AoideActorVector artists(
            int precedence = AoideActor::kPrecedenceSummary) const {
        return actors(AoideActor::kRoleArtist, precedence);
    }
    AoideActorVector actors(
            int role,
            int precedence = AoideActor::kPrecedenceSummary) const;
    AoideActorVector allActors() const;
    AoideActorVector removeActors(int role);
    AoideActorVector clearActors();
    void addActors(AoideActorVector actors);
};

Q_DECLARE_METATYPE(AoideTrackOrAlbum);

class AoideRelease : public AoideTrackOrAlbumOrRelease {
  public:
    explicit AoideRelease(QJsonObject jsonObject = QJsonObject())
            : AoideTrackOrAlbumOrRelease(std::move(jsonObject)) {
    }

    QString releasedAt() const;
    void setReleasedAt(const QString& releasedAt = QString());

    QString releasedBy() const;
    void setReleasedBy(QString label = QString());

    QString copyright() const;
    void setCopyright(QString copyright = QString());

    QStringList licenses() const;
    void setLicenses(QStringList licenses = QStringList());
};

Q_DECLARE_METATYPE(AoideRelease);

class AoideAlbum : public AoideTrackOrAlbum {
  public:
    explicit AoideAlbum(QJsonObject jsonObject = QJsonObject())
            : AoideTrackOrAlbum(std::move(jsonObject)) {
    }

    bool compilation(bool defaultValue = false) const;
    void setCompilation(bool compilation);
    void resetCompilation();
};

Q_DECLARE_METATYPE(AoideAlbum);

class AoideTrackCollection : public AoideJsonObject {
  public:
    explicit AoideTrackCollection(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    QString uid() const;
    void setUid(QString uid);

    QDateTime since() const;
    void setSince(QDateTime since = QDateTime());

    QString comment() const;
    void setComment(QString comment = QString());

    int playCount() const;
    void setPlayCount(int playCount);
};

Q_DECLARE_METATYPE(AoideTrackCollection);

class AoideTrackMarkers : public AoideJsonObject {
  public:
    explicit AoideTrackMarkers(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    AoidePositionMarkers positionMarkers() const;
    void setPositionMarkers(AoidePositionMarkers positionMarkers);

    AoideBeatMarkers beatMarkers() const;
    void setBeatMarkers(AoideBeatMarkers beatMarkers);

    AoideKeyMarkers keyMarkers() const;
    void setKeyMarkers(AoideKeyMarkers keyMarkers);
};

Q_DECLARE_METATYPE(AoideTrackMarkers);

class AoideTrack : public AoideTrackOrAlbum {
  public:
    explicit AoideTrack(QJsonObject jsonObject = QJsonObject())
            : AoideTrackOrAlbum(std::move(jsonObject)) {
    }

    AoideTrackCollection collection(const QString& uid) const;
    void setCollection(AoideTrackCollection collection);

    AoideMediaSource mediaSource(const QMimeType& contentType = QMimeType()) const;
    void setMediaSource(AoideMediaSource mediaSource);

    AoideRelease release() const;
    void setRelease(AoideRelease release);

    AoideAlbum album() const;
    void setAlbum(AoideAlbum album);

    QString trackNumbers() const;
    QString discNumbers() const;
    void setIndexNumbers(const mixxx::TrackInfo& trackInfo);

    AoideTags tags() const;
    AoideTags removeTags();
    void setTags(AoideTags tags);

    StarRating starRating() const;
    void setStarRating(const StarRating& starRating);

    AoideTrackMarkers markers() const;
    void setMarkers(AoideTrackMarkers markers);
};

Q_DECLARE_METATYPE(AoideTrack);

class AoideTrackEntity : public AoideJsonArray {
  public:
    explicit AoideTrackEntity(QJsonArray jsonArray = QJsonArray())
            : AoideJsonArray(std::move(jsonArray)) {
    }

    AoideEntityHeader header() const;

    AoideTrack body() const;
    void setBody(AoideTrack body);
};

Q_DECLARE_METATYPE(AoideTrackEntity);
