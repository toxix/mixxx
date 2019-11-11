#if defined(__AOIDE__)

#include <gtest/gtest.h>

#include <QtDebug>

#include "library/aoide/domain/track.h"
#include "library/aoide/tag/hashtagcommentstagger.h"

namespace mixxx {

namespace aoide {

class AoideHashtagCommentsTaggerTest : public testing::Test {
};

TEST_F(AoideHashtagCommentsTaggerTest, ExportImportTrack) {
    HashtagCommentsTagger tagger;

    AoideTags tags;
    tagger.exportCommentAsTags(
            &tags,
            "Any comments with ## and #hashtags that are not extracted##\n#Top40 #crate:DJ #epoch: 1990s #  example#facet  :  With this label  and many whitespaces  = 0.6 # another_facet:...with a label=0.1234 #rating:=0.85 #facet-with-default-score: #facet-with-label-eq:= #spotify:track:1f3yAtsJtY87CTmM8RLnxf");

    EXPECT_EQ(10, tags.allTags().size());
    EXPECT_EQ(1, tags.plainTags().size());
    EXPECT_TRUE(tags.plainTags()[0].isPlain());
    EXPECT_FALSE(tags.plainTags()[0].isFaceted());
    EXPECT_EQ("Top40", tags.plainTags()[0].label());
    EXPECT_EQ(9, tags.facetedTags().size());
    EXPECT_EQ(1, tags.facetedTags("crate").size());
    EXPECT_EQ(1, tags.facetedTags("epoch").size());
    EXPECT_EQ(AoideTag::defaultScore(), tags.facetedTags("epoch")[0].score());
    EXPECT_EQ("1990s", tags.facetedTags("epoch")[0].label());
    EXPECT_EQ(1, tags.facetedTags("rating").size());
    EXPECT_TRUE(tags.facetedTags("rating")[0].label().isEmpty());
    EXPECT_EQ(0.85, tags.facetedTags("rating")[0].score());
    EXPECT_EQ(1, tags.facetedTags("facet-with-default-score").size());
    EXPECT_TRUE(tags.facetedTags("facet-with-default-score")[0].label().isEmpty());
    EXPECT_EQ(AoideTag::defaultScore(), tags.facetedTags("facet-with-default-score")[0].score());
    EXPECT_EQ(1, tags.facetedTags("facet-with-label-eq").size());
    EXPECT_EQ("=", tags.facetedTags("facet-with-label-eq")[0].label());
    EXPECT_EQ(AoideTag::defaultScore(), tags.facetedTags("facet-with-label-eq")[0].score());
    EXPECT_EQ(1, tags.facetedTags("spotify").size());
    EXPECT_EQ(1, tags.facetedTags("example#facet").size());
    EXPECT_EQ("With this label  and many whitespaces", tags.facetedTags("example#facet")[0].label());
    EXPECT_EQ(0.6, tags.facetedTags("example#facet")[0].score());
    EXPECT_EQ(1, tags.facetedTags("another_facet").size());
    auto commentTags = tags.facetedTags(AoideTag::kFacetComment);
    EXPECT_EQ(1, commentTags.size());

    // ...and re-import the track to double check
    auto comment = HashtagCommentsTagger::importCommentFromTags(tags, true);
    EXPECT_EQ("Any comments with ## and #hashtags that are not extracted\n##\n#Top40\n#crate:DJ\n#epoch:1990s\n#example#facet:With this label  and many whitespaces=0.6\n#another_facet:...with a label=0.1234\n#rating:=0.85\n#facet-with-default-score:\n#facet-with-label-eq:=\n#spotify:track:1f3yAtsJtY87CTmM8RLnxf", comment);
}

} // namespace aoide

} // namespace mixxx

#endif
