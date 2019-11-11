#include "library/aoide/tag/hashtagcommentstagger.h"

#include <QRegularExpression>

#include "util/logger.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide HashtagCommentsTagger");

const QRegularExpression kRegexpWhitespace("\\s+");

const QString kCommentTagsSeparator = QStringLiteral("\n");

// All text preceding (= to the left of) this character sequence
// is protected as an individual comment tag and not considered
// when extracting hashtags.
// This separator also separates multiple protected comments if
// it occurs more than once. Hashtags will only be extracted from
// the substring after (= to the right of) the last separator.
const QString kProtectedCommentTagsSeparator = QStringLiteral("##\n");

const QString kHashtagPrefix = QStringLiteral("#");

const QString kFacetLabelSeparator = QStringLiteral(":");

const QString kLabelScoreSeparator = QStringLiteral("=");

AoideTag splitProtectedCommentTag(QString* comment) {
    int separatorIndex = comment->lastIndexOf(kProtectedCommentTagsSeparator);
    if (separatorIndex >= 0) {
        auto protectedComment = comment->left(separatorIndex).trimmed();
        // The part after the last separator may contain hashtags
        // and needs to be parsed subsequently.
        *comment = comment->right(comment->size() - (separatorIndex + kProtectedCommentTagsSeparator.size()));
        AoideTag protectedCommentTag;
        protectedCommentTag.setFacet(AoideTag::kFacetComment);
        protectedCommentTag.setLabel(std::move(protectedComment));
        return protectedCommentTag;
    }
    return AoideTag();
}

QString extractTagFacet(QString* token) {
    int separatorIndex = token->indexOf(kFacetLabelSeparator);
    if (separatorIndex >= 0) {
        QString facet = token->left(separatorIndex).trimmed();
        if (facet.isEmpty() || AoideTag::isValidFacet(facet)) {
            // Strip off consumed characters
            *token = token->right(token->size() - (separatorIndex + kFacetLabelSeparator.size()));
            return facet;
        }
    }
    return AoideTag::noFacet();
}

double extractTagScore(QString* token) {
    int separatorIndex = token->lastIndexOf(kLabelScoreSeparator);
    if (separatorIndex >= 0) {
        QString scoreString = token->right(token->size() - (separatorIndex + kLabelScoreSeparator.size()));
        bool isValidDouble = false;
        double score = scoreString.toDouble(&isValidDouble);
        if (isValidDouble && AoideTag::isValidScore(score)) {
            // Strip off consumed characters
            *token = token->left(separatorIndex);
            return score;
        }
    }
    return AoideTag::defaultScore();
}

AoideTag extractTag(QString token) {
    QString facet = extractTagFacet(&token);
    double score = extractTagScore(&token);
    QString label = token.trimmed();
    AoideTag tag;
    if (!facet.isEmpty() || !label.isEmpty()) {
        tag.setFacet(std::move(facet));
        tag.setLabel(std::move(label));
        tag.setScore(score);
    }
    return tag;
}

} // anonymous namespace

// static
QString HashtagCommentsTagger::importCommentFromTags(AoideTags tags, bool withHashtags) {
    // Breakup tags
    auto commentTags = tags.removeTags(AoideTag::kFacetComment);
    std::sort(commentTags.begin(), commentTags.end(), [](const AoideTag& t1, const AoideTag& t2) -> bool {
        return t1.score() > t2.score();
    });
    QStringList commentLabels;
    commentLabels.reserve(commentTags.size());
    for (auto&& tag : commentTags) {
        commentLabels += tag.label();
    }
    QString comment = commentLabels.join(kCommentTagsSeparator);
    if (!withHashtags) {
        return comment;
    }
    tags.removeTags(AoideTag::kFacetGenre);
    tags.removeTags(MixxxTag::kFacet, MixxxTag::kLabelHidden);
    tags.removeTags(MixxxTag::kFacet, MixxxTag::kLabelMissing);
    auto otherTags = tags.clearTags();
    if (!otherTags.empty()) {
        if (!comment.isEmpty() && comment.lastIndexOf('\n') < (comment.size() - 1)) {
            comment += '\n'; // line brake to improve readability
        }
        comment += kProtectedCommentTagsSeparator; // includes a line brake to improve readability
        AoideTagVector skippedTags;
        for (auto&& tag : otherTags) {
            if (comment.lastIndexOf('\n') < (comment.size() - 1)) {
                comment += '\n'; // line brake to improve readability
            }
            comment += kHashtagPrefix;
            DEBUG_ASSERT(tag.facet().trimmed() == tag.facet());
            if (!tag.facet().isEmpty()) {
                comment += tag.facet();
                comment += kFacetLabelSeparator;
            }
            comment += tag.label();
            if (tag.score() != AoideTag::defaultScore()) {
                comment += kLabelScoreSeparator;
                comment += QString().number(tag.score());
            }
        }
    }
    return comment;
}

namespace {
QStringList splitAndKeepSpaces(const QString& str) {
    QStringList tokens;
    QString nextSpaceToken;
    QString nextTextToken;
    for (int i = 0; i < str.size(); ++i) {
        auto c = str.at(i);
        if (c.isSpace()) {
            if (!nextTextToken.isEmpty()) {
                tokens += nextTextToken;
                nextTextToken.clear();
            }
            nextSpaceToken += c;
        } else {
            if (!nextSpaceToken.isEmpty()) {
                tokens += nextSpaceToken;
                nextSpaceToken.clear();
            }
            nextTextToken += c;
        }
    }
    if (!nextSpaceToken.isEmpty()) {
        tokens += nextSpaceToken;
    }
    if (!nextTextToken.isEmpty()) {
        tokens += nextTextToken;
    }
    return tokens;
}

AoideTagVector extractHashtagsFromText(QString* text) {
    auto tokens = splitAndKeepSpaces(*text);
    text->clear();
    AoideTagVector extractedTags;
    extractedTags.reserve(tokens.size());
    QString nextHashtag;
    tokens += QString(); // terminator
    for (auto token : tokens) {
        if (token.startsWith(kHashtagPrefix) || token.isEmpty()) {
            if (!nextHashtag.isEmpty()) {
                DEBUG_ASSERT(nextHashtag.size() >= kHashtagPrefix.size());
                auto extractedTag = extractTag(
                        nextHashtag.right(nextHashtag.size() - kHashtagPrefix.size()));
                if (extractedTag.isEmpty()) {
                    kLogger.info()
                            << "Cannot parse tag from #hashtag token:"
                            << nextHashtag;
                    if (!text->isEmpty() && !text->back().isSpace()) {
                        *text += ' ';
                    }
                    *text += nextHashtag;
                } else {
                    extractedTags += extractedTag;
                }
                nextHashtag.clear();
            }
            nextHashtag = token;
        } else {
            DEBUG_ASSERT((!token.isEmpty()));
            if (nextHashtag.isEmpty()) {
                if (!text->isEmpty() && !text->back().isSpace() && !token.front().isSpace()) {
                    *text += ' ';
                }
                *text += token;
            } else {
                nextHashtag += token;
            }
        }
    }
    DEBUG_ASSERT(nextHashtag.isEmpty()); // all consumed
    return extractedTags;
}
} // namespace

void HashtagCommentsTagger::exportCommentAsTags(AoideTags* tags, QString comment) const {
    DEBUG_ASSERT(tags);
    // 1st step: Add the protected comment part as a single tag
    auto protectedCommentTag = splitProtectedCommentTag(&comment);
    if (!protectedCommentTag.isEmpty()) {
        tags->addTags({std::move(protectedCommentTag)});
    }
    // 2nd step: Collect all hashtags from the unprotected comment part
    tags->addTags(extractHashtagsFromText(&comment));
    // 3rd step: Re-add the remaining, unparsed comment text as a single tag
    comment = comment.trimmed();
    if (!comment.isEmpty()) {
        AoideTag commentTag;
        commentTag.setFacet(AoideTag::kFacetComment);
        commentTag.setLabel(std::move(comment));
        tags->addTags({std::move(commentTag)});
    }
}

} // namespace aoide

} // namespace mixxx
