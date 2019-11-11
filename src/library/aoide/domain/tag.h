#pragma once

#include "library/aoide/domain/json.h"
#include "util/math.h"

class AoideTag : public AoideJsonArray {
  public:
    // Standard facets, i.e. for file tags
    static const QString kFacetContentGroup; // aka "Grouping"
    static const QString kFacetComment;
    static const QString kFacetGenre;
    static const QString kFacetMood;
    static const QString kFacetLanguage;
    static const QString kFacetRating;

    // General purpose facets
    static const QString kFacetCrate;
    static const QString kFacetCrowd;
    static const QString kFacetEpoch;
    static const QString kFacetEvent;
    static const QString kFacetStyle;
    static const QString kFacetVenue;

    // Audio feature facets
    static const QString kFacetAcousticness;
    static const QString kFacetDanceability;
    static const QString kFacetEnergy;
    static const QString kFacetInstrumentalness;
    static const QString kFacetLiveness;
    static const QString kFacetPopularity;
    static const QString kFacetSpeechiness;
    static const QString kFacetValence;

    // External link facets
    static const QString kFacetISRC;
    static const QString kFacetMusicBrainz;
    static const QString kFacetSpotify;

    AoideTag();

    static AoideTag fromPlain(QJsonValue jsonValue);
    static AoideTag fromFaceted(QJsonValue jsonValue);

    bool isEmpty() const;
    bool isValid() const;

    bool isPlain() const;
    bool isFaceted() const {
        return !isPlain();
    }

    static bool isValidFacet(const QString& facet);
    static QString noFacet() {
        return QString();
    }

    QString facet() const;
    void setFacet(QString facet);

    static bool isValidLabel(const QString& label);
    static QString noLabel() {
        return QString();
    }

    QString label() const;
    void setLabel(QString label);

    static bool isValidScore(double score) {
        return score >= 0.0 && score <= 1.0;
    }
    static double defaultScore() {
        return 1.0;
    }

    double score() const;
    void setScore(double score);

    QJsonValue intoJsonValue() override;
};

Q_DECLARE_METATYPE(AoideTag);

typedef QVector<AoideTag> AoideTagVector;

Q_DECLARE_METATYPE(AoideTagVector);

class MixxxTag {
  public:
    static const QString kFacet;

    static const QString kLabelRating;
    static const QString kLabelHidden;
    static const QString kLabelMissing;

    static AoideTag rating(double score);
    static AoideTag hidden();
    static AoideTag missing();
};

class AoideTags : public AoideJsonArray {
  public:
    explicit AoideTags(QJsonArray jsonArray = QJsonArray());

    AoideTagVector plainTags() const;
    AoideTagVector facetedTags(
            const QString& facet = AoideTag::noFacet(),
            const QString& label = AoideTag::noLabel()) const;
    AoideTagVector allTags() const;

    void addTags(AoideTagVector tags);
    AoideTagVector removeTags(
            const QString& facet = AoideTag::noFacet(),
            const QString& label = AoideTag::noLabel());
    AoideTagVector clearTags();
};

Q_DECLARE_METATYPE(AoideTags);

class AoideTagFacetCount : public AoideJsonObject {
  public:
    explicit AoideTagFacetCount(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    QString facet() const;

    quint64 count() const;
};

Q_DECLARE_METATYPE(AoideTagFacetCount);

class AoideTagCount : public AoideJsonObject {
  public:
    explicit AoideTagCount(QJsonObject jsonObject = QJsonObject())
            : AoideJsonObject(std::move(jsonObject)) {
    }

    // Tag with avg. score
    AoideTag tag() const;

    quint64 count() const;
};

Q_DECLARE_METATYPE(AoideTagCount);
