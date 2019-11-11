#pragma once

#include "library/aoide/domain/tag.h"

namespace mixxx {

namespace aoide {

class HashtagCommentsTagger {
  public:
    virtual ~HashtagCommentsTagger() = default;

    static QString importCommentFromTags(AoideTags tags, bool withHashtags = true);

    virtual void exportCommentAsTags(AoideTags* tags, QString comment) const;
};

} // namespace aoide

} // namespace mixxx
