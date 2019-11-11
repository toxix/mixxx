#pragma once

#include <QMetaType>
#include <QUrlQuery>

struct AoidePagination {
    quint64 offset = 0;
    quint64 limit = 0;

    void addToQuery(QUrlQuery* query) const;
};

Q_DECLARE_METATYPE(AoidePagination);
