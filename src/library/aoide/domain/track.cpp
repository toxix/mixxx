#include "library/aoide/domain/track.h"

#include "analyzer/analyzerebur128.h"
#include "track/trackinfo.h"
#include "util/assert.h"
#include "util/logger.h"
#include "util/math.h"

namespace {

const mixxx::Logger kLogger("aoide Track");

const int kMaxStarCount = 5;

QJsonValue optionalPositiveIntJsonValue(const QString& value) {
    bool valid = false;
    int intValue = value.toInt(&valid);
    if (valid && (intValue > 0)) {
        return QJsonValue(intValue);
    } else {
        return QJsonValue();
    }
}

} // anonymous namespace

QString AoideAudioEncoder::name() const {
    return m_jsonObject.value("n").toString();
}

void AoideAudioEncoder::setName(QString name) {
    putOptionalNonEmpty("n", std::move(name));
}

QString AoideAudioEncoder::settings() const {
    return m_jsonObject.value("s").toString();
}

void AoideAudioEncoder::setSettings(QString settings) {
    putOptionalNonEmpty("s", std::move(settings));
}

int AoideAudioContent::channelCount(int defaultCount) const {
    DEBUG_ASSERT(defaultCount >= 0);
    auto jsonValue = m_jsonObject.value("c");
    DEBUG_ASSERT(jsonValue.isDouble()); // layout not supported
    return jsonValue.toInt(defaultCount);
}

void AoideAudioContent::setChannelCount(int channelCount) {
    DEBUG_ASSERT(channelCount >= 0);
    m_jsonObject.insert("c", channelCount);
}

double AoideAudioContent::durationMs(double defaultMs) const {
    return m_jsonObject.value("d").toDouble(defaultMs);
}

void AoideAudioContent::setDurationMs(double durationMs) {
    m_jsonObject.insert("d", std::round(durationMs));
}

int AoideAudioContent::sampleRateHz(int defaultHz) const {
    DEBUG_ASSERT(defaultHz >= 0);
    return m_jsonObject.value("s").toInt(defaultHz);
}

void AoideAudioContent::setSampleRateHz(int sampleRateHz) {
    DEBUG_ASSERT(sampleRateHz >= 0);
    m_jsonObject.insert("s", sampleRateHz);
}

int AoideAudioContent::bitRateBps(int defaultBps) const {
    DEBUG_ASSERT(defaultBps >= 0);
    return m_jsonObject.value("b").toInt(defaultBps);
}

void AoideAudioContent::setBitRateBps(int bitRateBps) {
    DEBUG_ASSERT(bitRateBps >= 0);
    m_jsonObject.insert("b", bitRateBps);
}

double AoideAudioContent::loudnessLufs() const {
    return m_jsonObject.value("l").toDouble(std::nan("loudnessLufs"));
}

void AoideAudioContent::setLoudnessLufs(double loudnessLufs) {
    if (util_isnan(loudnessLufs)) {
        m_jsonObject.remove("l");
    } else {
        m_jsonObject.insert("l", loudnessLufs);
    }
}

void AoideAudioContent::resetLoudnessLufs() {
    m_jsonObject.remove("l");
}

mixxx::ReplayGain AoideAudioContent::replayGain() const {
    mixxx::ReplayGain replayGain;
    const auto lufs = loudnessLufs();
    if (!util_isnan(lufs)) {
        const auto referenceGainDb =
                AnalyzerEbur128::kReplayGain2ReferenceLUFS - lufs;
        replayGain.setRatio(db2ratio(referenceGainDb));
    }
    return replayGain;
}

void AoideAudioContent::setReplayGain(mixxx::ReplayGain replayGain) {
    if (replayGain.hasRatio()) {
        // Assumption: Gain has been calculated with the new EBU R128 algorithm.
        const double referenceGainDb = ratio2db(replayGain.getRatio());
        // Reconstruct the LUFS value from the relative gain
        const double ituBs1770Lufs = AnalyzerEbur128::kReplayGain2ReferenceLUFS - referenceGainDb;
        setLoudnessLufs(ituBs1770Lufs);
    } else {
        resetLoudnessLufs();
    }
}

AoideAudioEncoder AoideAudioContent::encoder() const {
    return AoideAudioEncoder(m_jsonObject.value("e").toObject());
}

void AoideAudioContent::setEncoder(AoideAudioEncoder encoder) {
    putOptionalNonEmpty("e", encoder.intoJsonValue());
}

EncodedUrl AoideMediaSource::uri() const {
    return EncodedUrl::fromUrlEncoded(m_jsonObject.value("u").toString().toUtf8());
}

void AoideMediaSource::setUri(const EncodedUrl& uri) {
    putOptionalNonEmpty("u", uri.toString());
}

QString AoideMediaSource::contentTypeName() const {
    return m_jsonObject.value("t").toString();
}

void AoideMediaSource::setContentType(QMimeType contentType) {
    putOptionalNonEmpty("t", contentType.name());
}

AoideAudioContent AoideMediaSource::audioContent() const {
    return AoideAudioContent(m_jsonObject.value("c").toObject().value("a").toObject());
}

void AoideMediaSource::setAudioContent(AoideAudioContent audioContent) {
    // Replace the entire content object
    m_jsonObject.insert("c", QJsonObject{{"a", audioContent.intoJsonValue()}});
}

AoideArtwork AoideMediaSource::artwork() const {
    return AoideArtwork(m_jsonObject.value("a").toObject());
}

void AoideMediaSource::setArtwork(AoideArtwork artwork) {
    putOptionalNonEmpty("a", artwork.intoJsonValue());
}

QSize AoideArtwork::size() const {
    const auto arr = m_jsonObject.value("s").toArray();
    if (arr.size() != 2) {
        return QSize();
    }
    const auto width = arr.at(0).toInt(-1);
    const auto height = arr.at(1).toInt(-1);
    if (width <= 0 || height <= 0) {
        return QSize();
    }
    return QSize(width, height);
}

void AoideArtwork::setSize(const QSize& size) {
    if (size.isValid() && !size.isEmpty()) {
        m_jsonObject.insert("s", QJsonArray{size.width(), size.height()});
    } else {
        m_jsonObject.remove("s");
    }
}

QString AoideArtwork::fingerprint() const {
    return m_jsonObject.value("f").toString();
}

void AoideArtwork::setFingerprint(QString fingerprint) {
    putOptionalNonEmpty("f", std::move(fingerprint));
}

EncodedUrl AoideArtwork::uri() const {
    return EncodedUrl::fromUrlEncoded(m_jsonObject.value("u").toString().toUtf8());
}

void AoideArtwork::setUri(const EncodedUrl& uri) {
    putOptionalNonEmpty("u", uri.toString());
}

QColor AoideArtwork::backgroundColor() const {
    return QColor(m_jsonObject.value("c").toString());
}

void AoideArtwork::setBackgroundColor(QColor backgroundColor) {
    putOptionalNonEmpty("c", formatColor(backgroundColor));
}

//static
QColor AoideArtwork::imageBackgroundColor(const QImage& image) {
    // The HSV histogram approach didn't work as expected. Nevertheless
    // the code is kept for future experiments.
    /*
    quint64 histogramH[256];
    std::fill(histogramH, histogramH + 256, 0);
    double sumS = 0.0;
    double sumV = 0.0;
    for (int i = 0; i < image.width(); ++i) {
        double innerSumS = 0.0;
        double innerSumV = 0.0;
        for (int j = 0; j < image.height(); ++j) {
            QColor color = image.pixelColor(i, j);
            qreal h, s, v;
            color.getHsvF(&h, &s, &v);
            ++histogramH[math_max(static_cast<int>(256.0 * h), 255)];
            innerSumS += s;
            innerSumV += v;
        }
        sumS += innerSumS / image.height();
        sumV += innerSumV / image.height();
    }
    qreal primaryH = 0.0;
    quint64 maxCount = 0;
    for (int i = 0; i < 256 - 8; ++i) {
        quint64 count = 0;
        for (int j = 0; j < 8; ++j) {
            count += histogramH[i + j];
        }
        if (maxCount < count) {
            maxCount = count;
            primaryH = i / (255.0 - 4.0);
        }
    }
    qreal avgS = sumS / image.width();
    qreal avgV = sumV / image.width();
    return QColor::fromHsvF(primaryH, avgS, avgV);
    */
    // Qt::SmoothTransformation is required for obtaining the average
    // color of the image! Otherwise the color of the single pixel
    // might just be sampled from a single pixel.
    return image.scaled(
                        1, 1, // single pixel
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation)
            .pixelColor(0, 0);
}

AoideTitle::AoideTitle(QJsonValue jsonValue) {
    if (jsonValue.isArray()) {
        m_jsonArray = jsonValue.toArray();
    } else if (jsonValue.isString()) {
        setName(jsonValue.toString());
    }
}

QString AoideTitle::name() const {
    return m_jsonArray.at(0).toString();
}

void AoideTitle::setName(QString name) {
    while (m_jsonArray.size() <= 0) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(0, std::move(name));
}

/*static*/ const int AoideTitle::kLevelMain = 0;
/*static*/ const int AoideTitle::kLevelSub = 1;
/*static*/ const int AoideTitle::kLevelWork = 2;
/*static*/ const int AoideTitle::kLevelMovement = 3;

int AoideTitle::level() const {
    return m_jsonArray.at(1).toInt(kLevelMain);
}

void AoideTitle::setLevel(int level) {
    while (m_jsonArray.size() <= 1) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(1, level);
}

QJsonValue AoideTitle::intoJsonValue() {
    if (level() == kLevelMain) {
        shrink(1);
    }
    if (m_jsonArray.size() == 1) {
        return m_jsonArray.first();
    } else {
        return AoideJsonArray::intoJsonValue();
    }
}

AoideActor::AoideActor(QJsonValue jsonValue) {
    if (jsonValue.isArray()) {
        m_jsonArray = jsonValue.toArray();
    } else if (jsonValue.isString()) {
        setName(jsonValue.toString());
    }
}

QString AoideActor::name() const {
    return m_jsonArray.at(0).toString();
}

void AoideActor::setName(QString name) {
    while (m_jsonArray.size() <= 0) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(0, std::move(name));
}

/*static*/ const int AoideActor::kRoleArtist = 0;
/*static*/ const int AoideActor::kRoleComposer = 2;
/*static*/ const int AoideActor::kRoleConductor = 3;
/*static*/ const int AoideActor::kRoleLyricist = 6;
/*static*/ const int AoideActor::kRoleRemixer = 11;

int AoideActor::role() const {
    return m_jsonArray.at(1).toInt(kRoleArtist);
}

void AoideActor::setRole(int role) {
    while (m_jsonArray.size() <= 1) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(1, role);
}

/*static*/ const int AoideActor::kPrecedenceSummary = 0;
/*static*/ const int AoideActor::kPrecedencePrimary = 1;
/*static*/ const int AoideActor::kPrecedenceSecondary = 1;

int AoideActor::precedence() const {
    return m_jsonArray.at(2).toInt(kPrecedenceSummary);
}

void AoideActor::setPrecedence(int precedence) {
    while (m_jsonArray.size() <= 2) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(2, precedence);
}

QJsonValue AoideActor::intoJsonValue() {
    if (precedence() == kPrecedenceSummary) {
        if (role() == kRoleArtist) {
            shrink(1);
        } else {
            shrink(2);
        }
    }
    if (m_jsonArray.size() == 1) {
        return m_jsonArray.first();
    } else {
        return AoideJsonArray::intoJsonValue();
    }
}

AoideTitleVector AoideTrackOrAlbum::titles(int level) const {
    AoideTitleVector result;
    const QJsonArray jsonTitles = m_jsonObject.value("t").toArray();
    for (auto&& jsonValue : jsonTitles) {
        auto title = AoideTitle(jsonValue);
        if (title.level() == level) {
            result += std::move(title);
        }
    }
    return result;
}

AoideTitleVector AoideTrackOrAlbum::allTitles() const {
    AoideTitleVector result;
    const QJsonArray jsonTitles = m_jsonObject.value("t").toArray();
    result.reserve(jsonTitles.size());
    for (auto&& jsonValue : jsonTitles) {
        result += AoideTitle(jsonValue);
    }
    return result;
}

AoideTitleVector AoideTrackOrAlbum::removeTitles(int level) {
    AoideTitleVector result;
    // NOTE(uklotzde, 2019-07-28): Modifying the QJsonArray in place
    // caused undefined behavior and crashes with Qt 5.12.4!
    const auto oldTitles = m_jsonObject.value("t").toArray();
    QJsonArray newTitles;
    for (const auto&& jsonValue : oldTitles) {
        auto title = AoideTitle(jsonValue);
        if (title.level() == level) {
            result += title;
        } else {
            newTitles += jsonValue;
        }
    }
    putOptionalNonEmpty("t", std::move(newTitles));
    return result;
}

AoideTitleVector AoideTrackOrAlbum::clearTitles() {
    AoideTitleVector result;
    const QJsonArray jsonTitles = m_jsonObject.take("t").toArray();
    result.reserve(jsonTitles.size());
    for (const auto&& jsonValue : jsonTitles) {
        result += AoideTitle(jsonValue);
    }
    return result;
}

void AoideTrackOrAlbum::addTitles(AoideTitleVector titles) {
    if (titles.isEmpty()) {
        // Avoid any modifications if noop
        return;
    }
    QJsonArray jsonTitles = m_jsonObject.take("t").toArray();
    for (auto&& title : titles) {
        jsonTitles += title.intoJsonValue();
    }
    putOptionalNonEmpty("t", std::move(jsonTitles));
}

AoideActorVector AoideTrackOrAlbum::actors(int role, int precedence) const {
    AoideActorVector result;
    const QJsonArray jsonActors = m_jsonObject.value("p").toArray();
    for (auto&& jsonValue : jsonActors) {
        auto actor = AoideActor(jsonValue);
        if ((actor.role() == role) && (actor.precedence() == precedence)) {
            result += std::move(actor);
        }
    }
    return result;
}

AoideActorVector AoideTrackOrAlbum::allActors() const {
    AoideActorVector result;
    const QJsonArray jsonActors = m_jsonObject.value("p").toArray();
    result.reserve(jsonActors.size());
    for (auto&& jsonValue : jsonActors) {
        result += AoideActor(jsonValue);
    }
    return result;
}

AoideActorVector AoideTrackOrAlbum::removeActors(int role) {
    AoideActorVector result;
    // NOTE(uklotzde, 2019-07-28): Modifying the QJsonArray in place
    // caused undefined behavior and crashes with Qt 5.12.4!
    const auto oldActors = m_jsonObject.value("p").toArray();
    QJsonArray newActors;
    for (const auto&& jsonValue : oldActors) {
        auto actor = AoideActor(jsonValue);
        if (actor.role() == role) {
            result += actor;
        } else {
            newActors += jsonValue;
        }
    }
    putOptionalNonEmpty("p", std::move(newActors));
    return result;
}

AoideActorVector AoideTrackOrAlbum::clearActors() {
    AoideActorVector result;
    const QJsonArray jsonActors = m_jsonObject.take("p").toArray();
    result.reserve(jsonActors.size());
    for (const auto&& jsonValue : jsonActors) {
        result += AoideActor(jsonValue);
    }
    return result;
}

void AoideTrackOrAlbum::addActors(AoideActorVector actors) {
    if (actors.isEmpty()) {
        // Avoid any modifications if noop
        return;
    }
    QJsonArray jsonActors = m_jsonObject.take("p").toArray();
    for (auto&& actor : actors) {
        jsonActors += actor.intoJsonValue();
    }
    putOptionalNonEmpty("p", std::move(jsonActors));
}

bool AoideAlbum::compilation(bool defaultValue) const {
    return m_jsonObject.value("c").toBool(defaultValue);
}

void AoideAlbum::setCompilation(bool compilation) {
    m_jsonObject.insert("c", compilation);
}

void AoideAlbum::resetCompilation() {
    m_jsonObject.remove("c");
}

QString AoideRelease::releasedAt() const {
    return importDateTimeOrYear(m_jsonObject.value("t"));
}

void AoideRelease::setReleasedAt(const QString& releasedAt) {
    putOptionalNonEmpty("t", exportDateTimeOrYear(releasedAt));
}

QString AoideRelease::releasedBy() const {
    return m_jsonObject.value("b").toString();
}

void AoideRelease::setReleasedBy(QString label) {
    putOptionalNonEmpty("b", std::move(label));
}

QString AoideRelease::copyright() const {
    return m_jsonObject.value("c").toString();
}

void AoideRelease::setCopyright(QString copyright) {
    putOptional("c", std::move(copyright));
}

QStringList AoideRelease::licenses() const {
    return toStringList(m_jsonObject.value("l").toArray());
}

void AoideRelease::setLicenses(QStringList licenses) {
    putOptionalNonEmpty("l", QJsonArray::fromStringList(std::move(licenses)));
}

QString AoideTrackCollection::uid() const {
    return m_jsonObject.value("u").toString();
}

void AoideTrackCollection::setUid(QString uid) {
    putOptionalNonEmpty("u", std::move(uid));
}

QDateTime AoideTrackCollection::since() const {
    const auto sinceVar = m_jsonObject.value("s").toVariant();
    if (sinceVar.isValid()) {
        bool ok;
        const auto sinceMicros = sinceVar.toULongLong(&ok);
        if (ok) {
            return QDateTime::fromMSecsSinceEpoch(sinceMicros / 1000);
        }
    }
    DEBUG_ASSERT(!"Invalid or missing time stamp");
    return QDateTime();
}

void AoideTrackCollection::setSince(QDateTime since) {
    putOptionalNonEmpty("s", exportDateTimeTicks(std::move(since)));
}

int AoideTrackCollection::playCount() const {
    return m_jsonObject.value("p").toInt(0);
}

void AoideTrackCollection::setPlayCount(int playCount) {
    if (playCount > 0) {
        m_jsonObject.insert("p", playCount);
    } else {
        m_jsonObject.remove("p");
    }
}

AoidePositionMarkers AoideTrackMarkers::positionMarkers() const {
    return AoidePositionMarkers(m_jsonObject.value("p").toObject());
}

void AoideTrackMarkers::setPositionMarkers(AoidePositionMarkers positionMarkers) {
    putOptionalNonEmpty("p", positionMarkers.intoJsonValue());
}

AoideBeatMarkers AoideTrackMarkers::beatMarkers() const {
    return AoideBeatMarkers(m_jsonObject.value("b").toObject());
}

void AoideTrackMarkers::setBeatMarkers(AoideBeatMarkers beatMarkers) {
    putOptionalNonEmpty("b", beatMarkers.intoJsonValue());
}

AoideKeyMarkers AoideTrackMarkers::keyMarkers() const {
    return AoideKeyMarkers(m_jsonObject.value("k").toObject());
}

void AoideTrackMarkers::setKeyMarkers(AoideKeyMarkers keyMarkers) {
    putOptionalNonEmpty("k", keyMarkers.intoJsonValue());
}

AoideMediaSource AoideTrack::mediaSource(const QMimeType& contentType) const {
    const QJsonArray mediaSources = m_jsonObject.value("s").toArray();
    VERIFY_OR_DEBUG_ASSERT((mediaSources.size() <= 1) || contentType.isValid()) {
        kLogger.warning()
                << "Missing content type for selecting one of"
                << mediaSources.size()
                << "media sources";
        return AoideMediaSource();
    }
    for (const auto& jsonValue : mediaSources) {
        const auto mediaSource = AoideMediaSource(jsonValue.toObject());
        if (!contentType.isValid() || (mediaSource.contentTypeName() == contentType.name())) {
            return mediaSource;
        }
    }
    DEBUG_ASSERT(mediaSources.isEmpty());
    kLogger.warning()
            << "No media source found for content type"
            << contentType;
    return AoideMediaSource();
}

void AoideTrack::setMediaSource(AoideMediaSource mediaSource) {
    putOptionalNonEmpty("s", QJsonArray{mediaSource.intoJsonValue()});
}

AoideTrackCollection AoideTrack::collection(const QString& uid) const {
    const auto collections = m_jsonObject.value("c");
    if (!collections.isArray()) {
        return AoideTrackCollection();
    }
    for (auto jsonObject : collections.toArray()) {
        const auto collection = AoideTrackCollection(jsonObject.toObject());
        if (collection.uid() == uid) {
            return collection;
        }
    }
    return AoideTrackCollection();
}

void AoideTrack::setCollection(AoideTrackCollection collection) {
    putOptionalNonEmpty("c", QJsonArray{collection.intoJsonValue()});
}

AoideRelease AoideTrack::release() const {
    return AoideRelease(m_jsonObject.value("r").toObject());
}

void AoideTrack::setRelease(AoideRelease release) {
    putOptionalNonEmpty("r", release.intoJsonValue());
}

AoideAlbum AoideTrack::album() const {
    return AoideAlbum(m_jsonObject.value("a").toObject());
}

void AoideTrack::setAlbum(AoideAlbum album) {
    putOptionalNonEmpty("a", album.intoJsonValue());
}

AoideTags AoideTrack::tags() const {
    return AoideTags(m_jsonObject.value("x").toArray());
}

AoideTags AoideTrack::removeTags() {
    return AoideTags(m_jsonObject.take("x").toArray());
}

void AoideTrack::setTags(AoideTags tags) {
    putOptionalNonEmpty("x", tags.intoJsonValue());
}

QString AoideTrack::trackNumbers() const {
    const auto value = m_jsonObject.value("i").toObject().value("t");
    if (value.isArray()) {
        auto array = value.toArray();
        DEBUG_ASSERT(array.size() == 2);
        return QString("%1/%2").arg(
                QString::number(array.at(0).toInt()),
                QString::number(array.at(1).toInt()));
    } else {
        return QString::number(value.toInt());
    }
}

QString AoideTrack::discNumbers() const {
    const auto value = m_jsonObject.value("i").toObject().value("d");
    if (value.isArray()) {
        auto array = value.toArray();
        DEBUG_ASSERT(array.size() == 2);
        return QString("%1/%2").arg(
                QString::number(array.at(0).toInt()),
                QString::number(array.at(1).toInt()));
    } else {
        return QString::number(value.toInt());
    }
}

void AoideTrack::setIndexNumbers(
        const mixxx::TrackInfo& trackInfo) {
    auto indexes = m_jsonObject.value("i").toObject();

    auto trackNumber = optionalPositiveIntJsonValue(trackInfo.getTrackNumber());
    auto trackTotal = optionalPositiveIntJsonValue(trackInfo.getTrackTotal());
    if (trackTotal.isNull()) {
        if (trackNumber.isNull()) {
            indexes.remove("t");
        } else {
            // Single value
            indexes.insert("t", std::move(trackNumber));
        }
    } else {
        // Tuple
        auto trackNumbers = QJsonArray{
                trackNumber.isNull() ? QJsonValue(0) : trackNumber,
                trackTotal,
        };
        indexes.insert("t", std::move(trackNumbers));
    }

    auto discNumber = optionalPositiveIntJsonValue(trackInfo.getDiscNumber());
    auto discTotal = optionalPositiveIntJsonValue(trackInfo.getDiscTotal());
    if (discTotal.isNull()) {
        if (discNumber.isNull()) {
            indexes.remove("d");
        } else {
            // Single value
            indexes.insert("d", std::move(discNumber));
        }
    } else {
        // Tuple
        auto discNumbers = QJsonArray{
                discNumber.isNull() ? QJsonValue(0) : discNumber,
                discTotal,
        };
        indexes.insert("d", std::move(discNumbers));
    }

    putOptionalNonEmpty("i", indexes);
}

StarRating AoideTrack::starRating() const {
    StarRating starRating(0, kMaxStarCount);
    AoideTagVector ratingTags = tags().facetedTags(MixxxTag::kFacet, MixxxTag::kLabelRating);
    DEBUG_ASSERT(ratingTags.size() <= 1);
    if (!ratingTags.isEmpty()) {
        starRating.setStarCount(static_cast<int>(
                std::round(ratingTags.first().score() * kMaxStarCount)));
    }
    return starRating;
}

void AoideTrack::setStarRating(const StarRating& starRating) {
    DEBUG_ASSERT(starRating.maxStarCount() == kMaxStarCount);
    double score = math_min(starRating.starCount(), starRating.maxStarCount()) /
            static_cast<double>(starRating.maxStarCount());
    auto tags = removeTags();
    tags.removeTags(MixxxTag::kFacet, MixxxTag::kLabelRating);
    tags.addTags({MixxxTag::rating(score)});
    setTags(std::move(tags));
}

AoideTrackMarkers AoideTrack::markers() const {
    return AoideTrackMarkers(m_jsonObject.value("m").toObject());
}

void AoideTrack::setMarkers(AoideTrackMarkers markers) {
    putOptionalNonEmpty("m", markers.intoJsonValue());
}

AoideEntityHeader AoideTrackEntity::header() const {
    return AoideEntityHeader(m_jsonArray.at(0).toArray());
}

AoideTrack AoideTrackEntity::body() const {
    return AoideTrack(m_jsonArray.at(1).toObject());
}

void AoideTrackEntity::setBody(AoideTrack body) {
    while (m_jsonArray.size() <= 1) {
        m_jsonArray.append(QJsonValue());
    }
    m_jsonArray.replace(1, body.intoJsonValue());
}
