#include "library/aoide/domain/tag.h"

#include <QRegularExpression>

#include "util/assert.h"

namespace {

const QRegularExpression kRegexpWhitespace("\\s+");

const QString kMixxxFacet = QStringLiteral("mixxx.org");

const QString kMixxxRatingLabel = QStringLiteral("rating");
const QString kMixxxHiddenLabel = QStringLiteral("hidden");
const QString kMixxxMissingLabel = QStringLiteral("missing");

const int kFacetIndex = 0;
const int kLabelIndex = 1;
const int kScoreIndex = 2;

} // anonymous namespace

/*static*/ const QString AoideTag::kFacetComment = "comment";
/*static*/ const QString AoideTag::kFacetContentGroup = "cgroup";
/*static*/ const QString AoideTag::kFacetGenre = "genre";
/*static*/ const QString AoideTag::kFacetLanguage = "lang";
/*static*/ const QString AoideTag::kFacetMood = "mood";
/*static*/ const QString AoideTag::kFacetRating = "rating";

/*static*/ const QString AoideTag::kFacetCrate = "crate";
/*static*/ const QString AoideTag::kFacetCrowd = "crowd";
/*static*/ const QString AoideTag::kFacetEpoch = "epoch";
/*static*/ const QString AoideTag::kFacetEvent = "event";
/*static*/ const QString AoideTag::kFacetStyle = "style";
/*static*/ const QString AoideTag::kFacetVenue = "venue";

/*static*/ const QString AoideTag::kFacetAcousticness = "acousticness";
/*static*/ const QString AoideTag::kFacetDanceability = "danceability";
/*static*/ const QString AoideTag::kFacetEnergy = "energy";
/*static*/ const QString AoideTag::kFacetInstrumentalness = "instrumentalness";
/*static*/ const QString AoideTag::kFacetLiveness = "liveness";
/*static*/ const QString AoideTag::kFacetPopularity = "popularity";
/*static*/ const QString AoideTag::kFacetSpeechiness = "speechiness";
/*static*/ const QString AoideTag::kFacetValence = "valence";

/*static*/ const QString AoideTag::kFacetISRC = "isrc";
/*static*/ const QString AoideTag::kFacetMusicBrainz = "musicbrainz";
/*static*/ const QString AoideTag::kFacetSpotify = "spotify";

/*static*/ const QString MixxxTag::kFacet = "mixxx.org";

/*static*/ const QString MixxxTag::kLabelRating = AoideTag::kFacetRating;
/*static*/ const QString MixxxTag::kLabelHidden = "hidden";
/*static*/ const QString MixxxTag::kLabelMissing = "missing";

AoideTag::AoideTag()
        : AoideJsonArray(QJsonArray{QJsonValue(), QJsonValue(), QJsonValue()}) {
}

AoideTag AoideTag::fromPlain(QJsonValue jsonValue) {
    AoideTag tag;
    if (jsonValue.isArray()) {
        // [label, score]
        auto jsonArray = jsonValue.toArray();
        DEBUG_ASSERT(jsonArray.size() == 2);
        DEBUG_ASSERT(jsonArray.at(0).isString());
        tag.setLabel(jsonArray.at(0).toString());
        DEBUG_ASSERT(jsonArray.at(1).isDouble());
        tag.setScore(jsonArray.at(1).toDouble());
    } else {
        // label
        DEBUG_ASSERT(jsonValue.isString());
        tag.setLabel(jsonValue.toString());
    }
    DEBUG_ASSERT(tag.isPlain());
    DEBUG_ASSERT(!tag.isFaceted());
    DEBUG_ASSERT(tag.m_jsonArray.size() == 3);
    return tag;
}

AoideTag AoideTag::fromFaceted(QJsonValue jsonValue) {
    AoideTag tag;
    if (jsonValue.isArray()) {
        auto jsonArray = jsonValue.toArray();
        DEBUG_ASSERT(jsonArray.size() >= 2);
        DEBUG_ASSERT(jsonArray.size() <= 3);
        DEBUG_ASSERT(jsonArray.at(0).isString());
        tag.setFacet(jsonArray.at(0).toString());
        if (jsonArray.size() == 2) {
            if (jsonArray.at(1).isDouble()) {
                // [facet, score]
                tag.setScore(jsonArray.at(1).toDouble());
            } else {
                // [facet, label]
                DEBUG_ASSERT(jsonArray.at(1).isString());
                tag.setLabel(jsonArray.at(1).toString());
            }
        } else {
            // [facet, label, score]
            DEBUG_ASSERT(jsonArray.size() == 3);
            DEBUG_ASSERT(jsonArray.at(1).isString());
            tag.setLabel(jsonArray.at(1).toString());
            DEBUG_ASSERT(jsonArray.at(2).isDouble());
            tag.setScore(jsonArray.at(2).toDouble());
        }
    } else {
        // facet
        DEBUG_ASSERT(jsonValue.isString());
        tag.setFacet(jsonValue.toString());
    }
    DEBUG_ASSERT(!tag.isPlain());
    DEBUG_ASSERT(tag.isFaceted());
    DEBUG_ASSERT(tag.m_jsonArray.size() == 3);
    return tag;
}

bool AoideTag::isEmpty() const {
    for (auto&& elem : m_jsonArray) {
        if (elem.isNull() || elem.isUndefined()) {
            continue;
        }
        return false;
    }
    return true;
}

bool AoideTag::isValid() const {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    return isPlain() ? !label().isEmpty() : !facet().isEmpty();
}

//static
bool AoideTag::isValidFacet(const QString& facet) {
    return facet.indexOf(kRegexpWhitespace) == -1 && facet.toLower() == facet;
}

bool AoideTag::isPlain() const {
    return facet().isEmpty();
}

QString AoideTag::facet() const {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    DEBUG_ASSERT(m_jsonArray.at(kFacetIndex).isString() || m_jsonArray.at(kFacetIndex).isNull());
    return m_jsonArray.at(kFacetIndex).toString();
}

void AoideTag::setFacet(QString facet) {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    DEBUG_ASSERT(facet.isEmpty() || isValidFacet(facet));
    m_jsonArray.replace(kFacetIndex, facet.isEmpty() ? QJsonValue() : QJsonValue(facet));
}

//static
bool AoideTag::isValidLabel(const QString& label) {
    return label.trimmed() == label;
}

QString AoideTag::label() const {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    DEBUG_ASSERT(m_jsonArray.at(kLabelIndex).isString() || m_jsonArray.at(kLabelIndex).isNull());
    return m_jsonArray.at(kLabelIndex).toString();
}

void AoideTag::setLabel(QString label) {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    DEBUG_ASSERT(label.isEmpty() || isValidLabel(label));
    m_jsonArray.replace(kLabelIndex, label.isEmpty() ? QJsonValue() : QJsonValue(label));
}

double AoideTag::score() const {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    if (m_jsonArray.at(2).isDouble()) {
        return m_jsonArray.at(kScoreIndex).toDouble(defaultScore());
    } else {
        return defaultScore();
    }
}

void AoideTag::setScore(double score) {
    DEBUG_ASSERT(m_jsonArray.size() == 3);
    DEBUG_ASSERT(util_isnan(score) || isValidScore(score));
    m_jsonArray.replace(kScoreIndex, score);
}

QJsonValue AoideTag::intoJsonValue() {
    if (isEmpty()) {
        return QJsonArray();
    }
    DEBUG_ASSERT(facet().isEmpty() || isValidFacet(facet()));
    DEBUG_ASSERT(label().isEmpty() || isValidLabel(label()));
    auto faceted = isFaceted();
    auto scored = isValidScore(score()) && score() != defaultScore();
    auto jsonArray = std::move(m_jsonArray);
    DEBUG_ASSERT(m_jsonArray.isEmpty());
    if (faceted) {
        // Remove empty label
        auto label = jsonArray.at(kLabelIndex).toString();
        if (label.isEmpty()) {
            jsonArray.removeAt(kLabelIndex);
        }
    } else {
        // Remove facet
        jsonArray.removeAt(kFacetIndex);
    }
    if (!scored) {
        jsonArray.removeLast();
    }
    DEBUG_ASSERT(!jsonArray.isEmpty());
    DEBUG_ASSERT(jsonArray.first().isString());
    DEBUG_ASSERT(!scored || jsonArray.last().isDouble());
    if (jsonArray.size() == 1) {
        return jsonArray.first();
    } else {
        return jsonArray;
    }
}

//static
AoideTag MixxxTag::rating(double score) {
    AoideTag tag;
    tag.setFacet(kFacet);
    tag.setLabel(kLabelRating);
    tag.setScore(score);
    return tag;
}

//static
AoideTag MixxxTag::hidden() {
    AoideTag tag;
    tag.setFacet(kFacet);
    tag.setLabel(kLabelHidden);
    return tag;
}

//static
AoideTag MixxxTag::missing() {
    AoideTag tag;
    tag.setFacet(kFacet);
    tag.setLabel(kLabelMissing);
    return tag;
}

AoideTags::AoideTags(QJsonArray jsonArray)
        : AoideJsonArray(std::move(jsonArray)) {
    if (m_jsonArray.isEmpty()) {
        m_jsonArray = QJsonArray{QJsonArray{}, QJsonArray{}};
    } else {
        DEBUG_ASSERT(m_jsonArray.size() == 2);
        DEBUG_ASSERT(m_jsonArray.at(0).isArray());
        DEBUG_ASSERT(m_jsonArray.at(1).isArray());
    }
}

AoideTagVector AoideTags::plainTags() const {
    AoideTagVector result;
    const QJsonArray jsonTags = m_jsonArray.at(0).toArray();
    result.reserve(jsonTags.size());
    for (const auto&& jsonValue : jsonTags) {
        result += AoideTag::fromPlain(jsonValue);
    }
    return result;
}

AoideTagVector AoideTags::facetedTags(
        const QString& facet,
        const QString& label) const {
    AoideTagVector result;
    const QJsonArray jsonTags = m_jsonArray.at(1).toArray();
    result.reserve(jsonTags.size());
    for (const auto&& jsonValue : jsonTags) {
        auto tag = AoideTag::fromFaceted(jsonValue);
        if ((facet == AoideTag::noFacet() || facet == tag.facet()) &&
                (label == AoideTag::noLabel() || label == tag.label())) {
            result += tag;
        }
    }
    return result;
}

AoideTagVector AoideTags::allTags() const {
    AoideTagVector result;
    const QJsonArray plain = m_jsonArray.at(0).toArray();
    const QJsonArray faceted = m_jsonArray.at(1).toArray();
    result.reserve(plain.size() + faceted.size());
    for (const auto&& jsonValue : plain) {
        result += AoideTag::fromPlain(jsonValue);
    }
    for (const auto&& jsonValue : faceted) {
        result += AoideTag::fromFaceted(jsonValue);
    }
    return result;
}

void AoideTags::addTags(AoideTagVector tags) {
    if (tags.isEmpty()) {
        // Avoid any modifications if noop
        return;
    }
    QJsonArray plain = m_jsonArray.at(0).toArray();
    QJsonArray faceted = m_jsonArray.at(1).toArray();
    for (auto&& tag : tags) {
        if (tag.isPlain()) {
            plain += tag.intoJsonValue();
        } else {
            DEBUG_ASSERT(tag.isFaceted());
            faceted += tag.intoJsonValue();
        }
    }
    m_jsonArray.replace(0, plain);
    m_jsonArray.replace(1, faceted);
}

AoideTagVector AoideTags::removeTags(
        const QString& facet,
        const QString& label) {
    AoideTagVector result;
    if (facet.isEmpty()) {
        const auto oldTags = m_jsonArray.at(0).toArray();
        m_jsonArray.replace(0, QJsonArray{});
        QJsonArray newTags;
        for (const auto&& jsonValue : oldTags) {
            auto tag = AoideTag::fromPlain(jsonValue);
            if (label == AoideTag::noLabel() || label == tag.label()) {
                result += tag;
            } else {
                newTags += jsonValue;
            }
        }
        m_jsonArray.replace(0, newTags);
    } else {
        DEBUG_ASSERT(facet != AoideTag::noFacet());
        const auto oldTags = m_jsonArray.at(1).toArray();
        m_jsonArray.replace(1, QJsonArray{});
        QJsonArray newTags;
        for (const auto&& jsonValue : oldTags) {
            auto tag = AoideTag::fromFaceted(jsonValue);
            if (facet == tag.facet()) {
                result += tag;
            } else {
                newTags += jsonValue;
            }
        }
        m_jsonArray.replace(1, newTags);
    }
    return result;
}

AoideTagVector AoideTags::clearTags() {
    AoideTagVector result;
    const auto plain = m_jsonArray.at(0).toArray();
    m_jsonArray.replace(0, QJsonArray{});
    const auto faceted = m_jsonArray.at(1).toArray();
    m_jsonArray.replace(1, QJsonArray{});
    result.reserve(plain.size() + faceted.size());
    for (const auto&& jsonValue : plain) {
        result += AoideTag::fromPlain(jsonValue);
    }
    for (const auto&& jsonValue : faceted) {
        result += AoideTag::fromFaceted(jsonValue);
    }
    return result;
}

QString AoideTagFacetCount::facet() const {
    return m_jsonObject.value("facet").toString();
}

quint64 AoideTagFacetCount::count() const {
    return m_jsonObject.value("count").toVariant().toULongLong();
}

AoideTag AoideTagCount::tag() const {
    QString facet = m_jsonObject.value("facet").toString();
    QString label = m_jsonObject.value("label").toString();
    double avgScore = m_jsonObject.value("avgScore").toDouble();
    AoideTag tag;
    tag.setFacet(facet);
    tag.setLabel(label);
    tag.setScore(avgScore);
    return tag;
}

quint64 AoideTagCount::count() const {
    return m_jsonObject.value("count").toVariant().toULongLong();
}
