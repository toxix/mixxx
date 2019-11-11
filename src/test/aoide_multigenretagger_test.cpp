#if defined(__AOIDE__)

#include <gtest/gtest.h>

#include <QtDebug>

#include "library/aoide/domain/track.h"
#include "library/aoide/settings.h"
#include "library/aoide/tag/multigenretagger.h"

namespace mixxx {

namespace aoide {

class AoideMultiGenreTaggerTest : public testing::Test {
};

TEST_F(AoideMultiGenreTaggerTest, ExportTrack) {
    Settings settings(UserSettingsPointer(new UserSettings(QDomNode())));
    settings.setMultiGenreAttenuation(0.5);
    settings.setMultiGenreSeparator(" - ");
    MultiGenreTagger tagger(settings);

    AoideTagVector genreTags =
            tagger.exportGenreTags("R&B/Soul - Pop - Hip-Hop/Rap - Rock'n'Roll - New Wave");

    EXPECT_EQ(5, genreTags.size());
    EXPECT_EQ("R&B/Soul", genreTags[0].label());
    EXPECT_EQ(1.0, genreTags[0].score());
    EXPECT_EQ("Pop", genreTags[1].label());
    EXPECT_EQ(0.5, genreTags[1].score());
    EXPECT_EQ("Hip-Hop/Rap", genreTags[2].label());
    EXPECT_EQ(0.25, genreTags[2].score());
    EXPECT_EQ("Rock'n'Roll", genreTags[3].label());
    EXPECT_EQ(0.125, genreTags[3].score());
    EXPECT_EQ("New Wave", genreTags[4].label());
    EXPECT_EQ(0.0625, genreTags[4].score());
}

TEST_F(AoideMultiGenreTaggerTest, ImportTrack) {
    Settings settings(UserSettingsPointer(new UserSettings(QDomNode())));
    settings.setMultiGenreAttenuation(0.5);

    AoideTagVector genreTags;
    {
        AoideTag genreTag;
        genreTag.setFacet(AoideTag::kFacetGenre);

        genreTag.setLabel("Pop");
        genreTag.setScore(0.7);
        genreTags += genreTag;

        genreTag.setLabel("New Wave");
        genreTag.setScore(0.1);
        genreTags += genreTag;

        genreTag.setLabel("R&B/Soul");
        genreTag.setScore(0.8);
        genreTags += genreTag;

        genreTag.setLabel("Hip-Hop/Rap");
        genreTag.setScore(0.67882);
        genreTags += genreTag;

        genreTag.setLabel("Rock'n'Roll");
        genreTag.setScore(0.4444);
        genreTags += genreTag;
    }

    ASSERT_EQ(5, genreTags.size());

    {
        // ambiguous result
        settings.setMultiGenreSeparator("-");
        MultiGenreTagger tagger(settings);
        EXPECT_EQ("R&B/Soul-Pop-Hip-Hop/Rap-Rock'n'Roll-New Wave", tagger.importGenre(genreTags));
    }
    {
        // unambiguous result
        settings.setMultiGenreSeparator("  ");
        MultiGenreTagger tagger(settings);
        auto genre = tagger.importGenre(genreTags);
        EXPECT_EQ("R&B/Soul  Pop  Hip-Hop/Rap  Rock'n'Roll  New Wave", genre);
        genreTags = tagger.exportGenreTags(genre);
        EXPECT_EQ(5, genreTags.size());
        EXPECT_EQ("R&B/Soul", genreTags[0].label());
        EXPECT_EQ(1.0, genreTags[0].score());
        EXPECT_EQ("Pop", genreTags[1].label());
        EXPECT_EQ(0.5, genreTags[1].score());
        EXPECT_EQ("Hip-Hop/Rap", genreTags[2].label());
        EXPECT_EQ(0.25, genreTags[2].score());
        EXPECT_EQ("Rock'n'Roll", genreTags[3].label());
        EXPECT_EQ(0.125, genreTags[3].score());
        EXPECT_EQ("New Wave", genreTags[4].label());
        EXPECT_EQ(0.0625, genreTags[4].score());
    }
}

} // namespace aoide

} // namespace mixxx

#endif
