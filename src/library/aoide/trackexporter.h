#pragma once

#include "library/aoide/domain/track.h"
#include "library/aoide/tag/multigenretagger.h"

class Track;

namespace mixxx {

namespace aoide {

class HashtagCommentsTagger;
class Settings;

class TrackExporter {
  public:
    TrackExporter(
            QString collectionUid,
            const Settings& settings);

    AoideTrack exportTrack(
            const Track& track,
            const HashtagCommentsTagger& commentsTagger) const;

  private:
    const QString m_collectionUid;
    const MultiGenreTagger m_genreTagger;
};

} // namespace aoide

} // namespace mixxx
