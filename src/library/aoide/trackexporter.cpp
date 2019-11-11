#include "library/aoide/trackexporter.h"

#include <QMimeDatabase>

#include "library/aoide/tag/hashtagcommentstagger.h"
#include "library/coverartutils.h"
#include "track/track.h"
#include "util/encodedurl.h"
#include "util/fingerprint.h"
#include "util/logger.h"
#include "util/math.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide TrackExporter");

inline void insertOptional(QJsonObject* jsonObject, const QString& key, QJsonObject value) {
    if (!value.isEmpty()) {
        jsonObject->insert(key, std::move(value));
    }
}

inline void insertOptional(QJsonObject* jsonObject, const QString& key, QJsonArray value) {
    if (!value.isEmpty()) {
        jsonObject->insert(key, std::move(value));
    }
}

const QString MBID_ARTIST_UUID_PREFIX = "artist/";
const QString MBID_RECORDING_UUID_PREFIX = "recording/";
const QString MBID_TRACK_UUID_PREFIX = "track/";
const QString MBID_WORK_UUID_PREFIX = "work/";
const QString MBID_RELEASE_UUID_PREFIX = "release/";
const QString MBID_RELEASE_GROUP_UUID_PREFIX = "release-group/";

inline void appendUri(
        QStringList* uris, const QString& uri, const QString& prefix = QString()) {
    if (!uri.isEmpty()) {
        *uris += prefix + uri;
    }
}

inline void appendUri(
        QStringList* uris,
        const QUuid& uuid, // e.g. MusicBrainz MBIDs
        const QString& prefix = QString()) {
    appendUri(uris, AoideJsonObject::formatUuid(uuid), prefix);
}

void appendTitle(AoideTitleVector* titles, QString name, int level) {
    if (!name.isEmpty()) {
        AoideTitle title;
        title.setName(std::move(name));
        title.setLevel(level);
        *titles += std::move(title);
    }
}

void appendActor(AoideActorVector* actors, QString name, int role) {
    if (!name.isEmpty()) {
        AoideActor actor;
        actor.setName(std::move(name));
        actor.setRole(role);
        *actors += std::move(actor);
    }
}

} // anonymous namespace

TrackExporter::TrackExporter(
        QString collectionUid,
        const Settings& settings)
        : m_collectionUid(std::move(collectionUid)),
          m_genreTagger(settings) {
    DEBUG_ASSERT(!m_collectionUid.isEmpty());
}

AoideTrack TrackExporter::exportTrack(
        const Track& track,
        const HashtagCommentsTagger& commentsTagger) const {
    TrackRecord trackRecord;
    track.readTrackRecord(&trackRecord);
    const TrackMetadata& trackMetadata = trackRecord.refMetadata();
    const TrackInfo& trackInfo = trackMetadata.getTrackInfo();
    const AlbumInfo& albumInfo = trackMetadata.getAlbumInfo();

    AoideTrack aoideTrack;
    aoideTrack.setIndexNumbers(trackInfo);

    AoideTrackCollection trackCollection;
    trackCollection.setUid(m_collectionUid);
    trackCollection.setPlayCount(trackRecord.getPlayCounter().getTimesPlayed());
    DEBUG_ASSERT(trackRecord.getDateAdded().isValid());
    trackCollection.setSince(trackRecord.getDateAdded());
    aoideTrack.setCollection(std::move(trackCollection));

    AoideAudioContent audioContent;
    DEBUG_ASSERT(!util_isnan(track.getDuration()));
    if (track.getDuration() > 0) {
        audioContent.setDurationMs(track.getDuration() * 1000);
    } else {
        kLogger.warning()
                << track.getFileInfo().location()
                << "has invalid or unknown duration:"
                << track.getDuration();
    }
    if (track.getChannels() > 0) {
        audioContent.setChannelCount(track.getChannels());
    } else {
        kLogger.warning()
                << track.getFileInfo().location()
                << "has invalid or unknown number of channels:"
                << track.getChannels();
    }
    if (track.getSampleRate() > 0) {
        audioContent.setSampleRateHz(track.getSampleRate());
    } else {
        kLogger.warning()
                << track.getFileInfo().location()
                << "has invalid or unknown sample rate:"
                << track.getSampleRate();
    }
    if (track.getBitrate() > 0) {
        audioContent.setBitRateBps(track.getBitrate() * 1000);
    } else {
        kLogger.warning()
                << track.getFileInfo().location()
                << "has invalid or unknown bit rate:"
                << track.getBitrate();
    }
    audioContent.setReplayGain(trackInfo.getReplayGain());

    AoideAudioEncoder audioEncoder;
    audioEncoder.setName(trackInfo.getEncoder());
    audioEncoder.setSettings(trackInfo.getEncoderSettings());
    audioContent.setEncoder(std::move(audioEncoder));

    // The media source property is only valid for existing tracks
    // with a valid URL and valid content type!
    AoideMediaSource mediaSource;
    const auto trackFile = track.getFileInfo();
    DEBUG_ASSERT(trackFile.toUrl().isValid());
    mediaSource.setUri(EncodedUrl::fromUrl(trackFile.toUrl()));
    const auto trackLocation = trackFile.location();
    DEBUG_ASSERT(!trackLocation.isEmpty());
    const auto contentType = QMimeDatabase().mimeTypeForFile(trackLocation);
    DEBUG_ASSERT(contentType.isValid());
    mediaSource.setContentType(contentType);
    mediaSource.setAudioContent(std::move(audioContent));
    // Artwork
    const auto coverInfo = track.getCoverInfoWithLocation();
    if (coverInfo.type != CoverInfoRelative::NONE) {
        QImage image = CoverArtUtils::loadCover(coverInfo);
        if (!image.isNull() || image.size().isEmpty()) {
            AoideArtwork artwork;
            artwork.setSize(
                    image.size());
            artwork.setFingerprint(
                    encodeFingerprint(hashImage(image)));
            artwork.setBackgroundColor(
                    AoideArtwork::imageBackgroundColor(image));
            if (coverInfo.type == CoverInfoRelative::FILE) {
                const auto url = QUrl::fromLocalFile(coverInfo.coverLocation);
                artwork.setUri(EncodedUrl::fromUrl(url));
            }
            mediaSource.setArtwork(std::move(artwork));
        }
    }
    aoideTrack.setMediaSource(std::move(mediaSource));

    AoideTrackMarkers trackMarkers;

    // Position markers
    const auto cuePositionToMillisDenom = track.getSampleRate() * track.getChannels();
    if (cuePositionToMillisDenom > 0) {
        // The cue positions are measured in samples of an interleaved PCM signal,
        // i.e. their time offset depends on both the sample rate and the number
        // of channels of the signal.
        const auto cuePositionToMillis = 1000.0 / cuePositionToMillisDenom;
        const auto&& cuePoints = track.getCuePoints();
        QVector<AoidePositionMarker> markers;
        markers.reserve(cuePoints.size());
        for (const auto& cuePoint : cuePoints) {
            auto positionMarker =
                    AoidePositionMarker::fromCue(
                            *cuePoint,
                            cuePositionToMillis);
            markers.append(positionMarker);
        }
        AoidePositionMarkers positionMarkers;
        positionMarkers.setMarkers(markers);
        trackMarkers.setPositionMarkers(positionMarkers);
    } else {
        kLogger.warning()
                << "Unable to export cue points of track"
                << track.getFileInfo().location();
    }

    // Beat marker(s)/grid
    if (trackInfo.getBpm().hasValue()) {
        AoideBeatMarker beatMarker;
        beatMarker.setStart(0.0);
        beatMarker.setTempoBpm(trackInfo.getBpm().getValue());
        AoideBeatMarkers beatMarkers;
        beatMarkers.setLocked(trackRecord.getBpmLocked());
        beatMarkers.setMarkers(QVector<AoideBeatMarker>{beatMarker});
        trackMarkers.setBeatMarkers(beatMarkers);
    }

    // Key marker(s)/grid
    if (trackRecord.getGlobalKey() != mixxx::track::io::key::ChromaticKey::INVALID) {
        AoideKeyMarker keyMarker;
        keyMarker.setStart(0.0);
        keyMarker.setKey(trackRecord.getGlobalKey());
        AoideKeyMarkers keyMarkers;
        keyMarkers.setMarkers(QVector<AoideKeyMarker>{keyMarker});
        trackMarkers.setKeyMarkers(keyMarkers);
    }

    aoideTrack.setMarkers(std::move(trackMarkers));

    // Track titles
    AoideTitleVector trackTitles;
    appendTitle(&trackTitles, trackInfo.getTitle(), AoideTitle::kLevelMain);
    appendTitle(&trackTitles, trackInfo.getSubtitle(), AoideTitle::kLevelSub);
    appendTitle(&trackTitles, trackInfo.getWork(), AoideTitle::kLevelWork);
    appendTitle(&trackTitles, trackInfo.getMovement(), AoideTitle::kLevelMovement);
    aoideTrack.addTitles(std::move(trackTitles));

    // Track actors
    AoideActorVector trackActors;
    appendActor(&trackActors, trackInfo.getArtist(), AoideActor::kRoleArtist);
    appendActor(&trackActors, trackInfo.getComposer(), AoideActor::kRoleComposer);
    appendActor(&trackActors, trackInfo.getConductor(), AoideActor::kRoleConductor);
    appendActor(&trackActors, trackInfo.getLyricist(), AoideActor::kRoleLyricist);
    appendActor(&trackActors, trackInfo.getRemixer(), AoideActor::kRoleRemixer);
    aoideTrack.addActors(std::move(trackActors));

    // Album {
    AoideAlbum aoideAlbum = aoideTrack.album();

    // Album titles
    AoideTitleVector albumTitles;
    appendTitle(&albumTitles, albumInfo.getTitle(), AoideTitle::kLevelMain);
    aoideAlbum.addTitles(std::move(albumTitles));

    // Album actors
    AoideActorVector albumActors;
    appendActor(&albumActors, albumInfo.getArtist(), AoideActor::kRoleArtist);
    aoideAlbum.addActors(std::move(albumActors));

    aoideTrack.setAlbum(std::move(aoideAlbum));
    // } Album

    // Release
    AoideRelease release = aoideTrack.release();
    release.setReleasedAt(trackInfo.getYear());
    release.setReleasedBy(albumInfo.getRecordLabel());
    release.setCopyright(albumInfo.getCopyright());
    if (!albumInfo.getLicense().isEmpty()) {
        release.setLicenses({albumInfo.getLicense()});
    }
    aoideTrack.setRelease(std::move(release));

    // Tags
    AoideTags tags;

    tags.addTags(m_genreTagger.exportGenreTags(trackInfo.getGenre()));
    if (!trackInfo.getLanguage().trimmed().isEmpty()) {
        AoideTag tag;
        tag.setFacet(AoideTag::kFacetLanguage);
        tag.setLabel(trackInfo.getLanguage().trimmed());
        tags.addTags({std::move(tag)});
    }
    if (!trackInfo.getGrouping().trimmed().isEmpty()) {
        AoideTag tag;
        tag.setFacet(AoideTag::kFacetContentGroup);
        tag.setLabel(trackInfo.getGrouping().trimmed());
        tags.addTags({std::move(tag)});
    }
    if (!trackInfo.getMood().trimmed().isEmpty()) {
        AoideTag tag;
        tag.setFacet(AoideTag::kFacetMood);
        tag.setLabel(trackInfo.getMood().trimmed());
        tags.addTags({std::move(tag)});
    }
    if (!trackInfo.getComment().trimmed().isEmpty()) {
        commentsTagger.exportCommentAsTags(&tags, trackInfo.getComment().trimmed());
    }

    // ISRC
    if (!trackInfo.getISRC().trimmed().isEmpty()) {
        AoideTag tag;
        tag.setFacet(AoideTag::kFacetISRC);
        tag.setLabel(trackInfo.getISRC().trimmed());
        tags.addTags({std::move(tag)});
    }

    // MusicBrainz
    QStringList musicBrainzLabels;
    appendUri(&musicBrainzLabels, trackInfo.getMusicBrainzRecordingId(), MBID_RECORDING_UUID_PREFIX);
    appendUri(&musicBrainzLabels, trackInfo.getMusicBrainzReleaseId(), MBID_TRACK_UUID_PREFIX);
    appendUri(&musicBrainzLabels, trackInfo.getMusicBrainzWorkId(), MBID_WORK_UUID_PREFIX);
    appendUri(&musicBrainzLabels, trackInfo.getMusicBrainzArtistId(), MBID_ARTIST_UUID_PREFIX);
    appendUri(&musicBrainzLabels, albumInfo.getMusicBrainzReleaseGroupId(), MBID_RELEASE_GROUP_UUID_PREFIX);
    appendUri(&musicBrainzLabels, albumInfo.getMusicBrainzReleaseId(), MBID_RELEASE_UUID_PREFIX);
    if (albumInfo.getMusicBrainzArtistId() != trackInfo.getMusicBrainzArtistId()) {
        appendUri(&musicBrainzLabels, albumInfo.getMusicBrainzArtistId(), MBID_ARTIST_UUID_PREFIX);
    }
    for (auto&& label : musicBrainzLabels) {
        AoideTag tag;
        tag.setFacet(AoideTag::kFacetMusicBrainz);
        tag.setLabel(std::move(label));
        tags.addTags({std::move(tag)});
    }

    aoideTrack.setTags(std::move(tags));

    if (trackRecord.hasRating()) {
        aoideTrack.setStarRating(trackRecord.getRating());
    }

    return aoideTrack;
} // namespace aoide

} // namespace aoide

} // namespace mixxx
