#pragma once

#include "library/aoide/domain/tag.h"

namespace mixxx {

namespace aoide {

class Settings;

class MultiGenreTagger {
  public:
    explicit MultiGenreTagger(
            const Settings& settings);

    QString importGenre(AoideTagVector genreTags) const;

    AoideTagVector exportGenreTags(const QString& genre) const;

  private:
    const QString m_multiGenreSeparator;
    const double m_multiGenreAttenuation;
};

} // namespace aoide

} // namespace mixxx
