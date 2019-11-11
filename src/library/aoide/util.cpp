#include "library/aoide/util.h"

#include "util/assert.h"

void AoidePagination::addToQuery(QUrlQuery* query) const {
    DEBUG_ASSERT(query);
    if (offset > 0) {
        query->addQueryItem("offset", QString::number(offset));
    }
    if (limit > 0) {
        query->addQueryItem("limit", QString::number(limit));
    }
}
