#if defined(__AOIDE__)

#include <gtest/gtest.h>

#include <QtDebug>

#include "library/aoide/tag/hashtagcommentstagger.h"
#include "library/aoide/trackexporter.h"
#include "library/aoide/settings.h"

#include "track/track.h"

const QDir kTestDir(QDir::current().absoluteFilePath("src/test/id3-test-data"));

namespace mixxx {

namespace aoide {

class AoideTrackExporterTest : public testing::Test {
  protected:
    void exportDateTimeOrYear(const QString& input, const QJsonValue& expectedOutput) {
        const auto actualOutput =
                QJsonValue::fromVariant(AoideJsonObject::exportDateTimeOrYear(input));
        if (actualOutput != expectedOutput) {
            qWarning() << "expected =" << expectedOutput << ", actual =" << actualOutput;
            EXPECT_EQ(expectedOutput, actualOutput);
        }
    }
};

TEST_F(AoideTrackExporterTest, exportDateTimeOrYear) {
    // Unmodified
    exportDateTimeOrYear("2018-01-01T01:02:03.457Z", "2018-01-01T01:02:03.457Z");
    exportDateTimeOrYear("2018-01-01T01:02:03.457+02:00", "2018-01-01T01:02:03.457+02:00");

    // Round to milliseconds
    exportDateTimeOrYear("2018-01-01T01:02:03.45678Z", "2018-01-01T01:02:03.457Z");
    exportDateTimeOrYear("2018-01-01T01:02:03.45678+02:00", "2018-01-01T01:02:03.457+02:00");

    // Strip zero milliseconds
    exportDateTimeOrYear("2018-04-27T07:00:00.000Z", "2018-04-27T07:00:00Z");
    exportDateTimeOrYear("2018-04-27T07:00:00.000-06:00", "2018-04-27T07:00:00-06:00");

    // Without milliseconds
    exportDateTimeOrYear("2018-04-27T07:00:00Z", "2018-04-27T07:00:00Z");
    exportDateTimeOrYear("2018-04-27T07:00:00-06:00", "2018-04-27T07:00:00-06:00");

    // Missing time zone or spec -> assume UTC
    exportDateTimeOrYear("2018-04-27T07:00:00.123", "2018-04-27T07:00:00.123Z");
    exportDateTimeOrYear("2018-04-27T07:00:00", "2018-04-27T07:00:00Z");

    // Missing time zone or spec and missing seconds -> assume UTC
    exportDateTimeOrYear("2018-04-27T07:00", "2018-04-27T07:00:00Z");

    // Space-separated and missing time zone or spec -> assume UTC
    exportDateTimeOrYear("2018-12-08 04:28:16", "2018-12-08T04:28:16Z");
    exportDateTimeOrYear("2018-12-21 05:59", "2018-12-21T05:59:00Z");

    // Only a date without a time
    exportDateTimeOrYear("\t2007-11-16", 20071116);
    exportDateTimeOrYear("1996-01-01\n", 19960101);
    exportDateTimeOrYear("1989- 3- 9", 19890309);

    // Only a year + month
    exportDateTimeOrYear("2007-11 ", 20071100);
    exportDateTimeOrYear(" 2007- 4", 20070400);

    // Only a year
    exportDateTimeOrYear(" 2007 ", 20070000);
}

TEST_F(AoideTrackExporterTest, ExportTrack) {
    const QFileInfo testFile(kTestDir.absoluteFilePath("cover-test.flac"));
    ASSERT_TRUE(testFile.exists());

    TrackPointer trackPtr = Track::newTemporary(TrackFile(testFile));

    trackPtr->setTitle("Track Title");
    trackPtr->setArtist("Track Artist");
    trackPtr->setAlbum("Album Title");
    trackPtr->setAlbumArtist("Album Artist");
    trackPtr->setGenre("Genre");
    trackPtr->setComment("Comment");
    trackPtr->setRating(3);

    const QString collectionUid = "collection1";
    trackPtr->setDateAdded(QDateTime::currentDateTime());

    Settings settings(UserSettingsPointer(new UserSettings(QDomNode())));
    AoideTrack aoideTrack = TrackExporter(collectionUid, settings).exportTrack(*trackPtr, HashtagCommentsTagger());

    auto tags = aoideTrack.removeTags();
    AoideTag freeTag;
    freeTag.setLabel("A free tag");
    tags.addTags({freeTag});
    aoideTrack.setTags(std::move(tags));

    EXPECT_EQ(1, aoideTrack.allTitles().size());
    EXPECT_EQ(1, aoideTrack.titles().size());
    EXPECT_EQ(trackPtr->getTitle(), aoideTrack.titles().first().name());
    EXPECT_EQ(1, aoideTrack.allActors().size());
    EXPECT_EQ(1, aoideTrack.actors(AoideActor::kRoleArtist).size());
    EXPECT_EQ(trackPtr->getArtist(), aoideTrack.actors(AoideActor::kRoleArtist).first().name());
    EXPECT_EQ(1, aoideTrack.album().allTitles().size());
    EXPECT_EQ(1, aoideTrack.album().titles().size());
    EXPECT_EQ(trackPtr->getAlbum(), aoideTrack.album().titles().first().name());
    EXPECT_EQ(1, aoideTrack.album().allActors().size());
    EXPECT_EQ(1, aoideTrack.album().actors(AoideActor::kRoleArtist).size());
    EXPECT_EQ(
            trackPtr->getAlbumArtist(),
            aoideTrack.album().actors(AoideActor::kRoleArtist).first().name());

    // Tags
    EXPECT_EQ(4, aoideTrack.tags().allTags().size());
    EXPECT_EQ(1, aoideTrack.tags().facetedTags(AoideTag::kFacetGenre).size());
    EXPECT_EQ(trackPtr->getGenre(), aoideTrack.tags().facetedTags(AoideTag::kFacetGenre).first().label());
    EXPECT_EQ(1.0, aoideTrack.tags().facetedTags(AoideTag::kFacetGenre).first().score());
    EXPECT_EQ(1, aoideTrack.tags().facetedTags(AoideTag::kFacetComment).size());
    EXPECT_EQ(trackPtr->getComment(), aoideTrack.tags().facetedTags(AoideTag::kFacetComment).first().label());
    EXPECT_EQ(1, aoideTrack.tags().facetedTags(MixxxTag::kFacet, MixxxTag::kLabelRating).size());
    EXPECT_NEAR(trackPtr->getRating() / 5.0, aoideTrack.tags().facetedTags(MixxxTag::kFacet, MixxxTag::kLabelRating).first().score(), 1e-6);
    EXPECT_EQ(1, aoideTrack.tags().plainTags().size());
    EXPECT_EQ(freeTag.label(), aoideTrack.tags().plainTags().first().label());

    aoideTrack.removeTitles(AoideTitle::kLevelMain);
    AoideTitle trackTitle;
    trackTitle.setName("New Track Title");
    aoideTrack.addTitles({trackTitle});
    EXPECT_EQ(1, aoideTrack.titles().size());
    EXPECT_EQ(AoideTitle::kLevelMain, aoideTrack.titles().first().level());
    EXPECT_EQ(trackTitle.name(), aoideTrack.titles().first().name());

    aoideTrack.removeActors(AoideActor::kRoleArtist);
    AoideActor trackArtist;
    trackArtist.setRole(AoideActor::kRoleArtist);
    trackArtist.setName("New Track Artist");
    aoideTrack.addActors({trackArtist});
    EXPECT_EQ(1, aoideTrack.actors(AoideActor::kRoleArtist).size());
    EXPECT_EQ(
            AoideActor::kPrecedenceSummary,
            aoideTrack.actors(AoideActor::kRoleArtist).first().precedence());
    EXPECT_EQ(trackArtist.name(), aoideTrack.actors(AoideActor::kRoleArtist).first().name());

    AoideAlbum album = aoideTrack.album();

    album.removeTitles(AoideTitle::kLevelMain);
    AoideTitle albumTitle;
    albumTitle.setName("New Album Title");
    album.addTitles({albumTitle});
    EXPECT_EQ(1, album.titles().size());
    EXPECT_EQ(AoideTitle::kLevelMain, album.titles().first().level());
    EXPECT_EQ(albumTitle.name(), album.titles().first().name());

    album.removeActors(AoideActor::kRoleArtist);
    AoideActor albumArtist;
    albumArtist.setRole(AoideActor::kRoleArtist);
    albumArtist.setName("New Album Artist");
    album.addActors({albumArtist});
    EXPECT_EQ(1, album.actors(AoideActor::kRoleArtist).size());
    EXPECT_EQ(
            AoideActor::kPrecedenceSummary,
            album.actors(AoideActor::kRoleArtist).first().precedence());
    EXPECT_EQ(albumArtist.name(), album.actors(AoideActor::kRoleArtist).first().name());
}

} // namespace aoide

} // namespace mixxx

#endif
