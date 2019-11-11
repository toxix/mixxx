#include "library/aoide/tag/multigenretagger.h"

#include <algorithm>

#include "library/aoide/settings.h"
#include "util/logger.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide MultiGenreTagger");

} // anonymous namespace

MultiGenreTagger::MultiGenreTagger(const Settings& settings)
        : m_multiGenreSeparator(settings.multiGenreSeparator()),
          m_multiGenreAttenuation(settings.multiGenreAttenuation()) {
    DEBUG_ASSERT(!m_multiGenreSeparator.isEmpty());
    DEBUG_ASSERT(m_multiGenreAttenuation > 0);
    DEBUG_ASSERT(m_multiGenreAttenuation <= 1);
}

QString MultiGenreTagger::importGenre(AoideTagVector genreTags) const {
    if (genreTags.isEmpty()) {
        return QString();
    }
    if (genreTags.size() == 1) {
        return genreTags.first().label();
    }
    std::sort(genreTags.begin(), genreTags.end(), [](const AoideTag& t1, const AoideTag& t2) -> bool {
        return t1.score() > t2.score();
    });
    QStringList genreLabels;
    for (auto&& tag : genreTags) {
        auto genreLabel = tag.label();
        DEBUG_ASSERT(!genreLabel.trimmed().isEmpty());
        if (genreLabel.contains(m_multiGenreSeparator)) {
            kLogger.warning()
                    << "Multi-genre separator"
                    << m_multiGenreSeparator
                    << "is ambiguous for genre"
                    << genreLabel;
        }
        genreLabels += genreLabel;
    }
    return genreLabels.join(m_multiGenreSeparator);
}

AoideTagVector MultiGenreTagger::exportGenreTags(const QString& genre) const {
    QStringList genres = genre.split(m_multiGenreSeparator, QString::SkipEmptyParts);
    AoideTagVector genreTags;
    genreTags.reserve(genres.size());
    double score = AoideTag::defaultScore();
    for (auto&& genre : genres) {
        genre = genre.trimmed();
        if (!genre.isEmpty()) {
            AoideTag genreTag;
            genreTag.setFacet(AoideTag::kFacetGenre);
            genreTag.setLabel(genre);
            genreTag.setScore(score);
            genreTags += std::move(genreTag);
            score *= m_multiGenreAttenuation;
            DEBUG_ASSERT(score > 0);
        }
    }
    return genreTags;
}

} // namespace aoide

} // namespace mixxx
