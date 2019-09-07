#pragma once

#include <QMetaType>

#include "util/assert.h"

namespace mixxx {

namespace network {

class RequestId final {
  public:
    typedef quint32 value_t;

    static void registerMetaType();

    // Thread-safe generation of a new, valid request id that is
    // unique within the context of the current process.
    static RequestId nextValid();

    RequestId()
            : m_value(0) {
        DEBUG_ASSERT(!isValid());
    }

    bool isValid() const {
        return m_value != 0;
    }

    void reset() {
        *this = RequestId();
        DEBUG_ASSERT(!isValid());
    }

    operator value_t() const {
        return m_value;
    }

  private:
    explicit RequestId(value_t value)
            : m_value(value) {
    }

    value_t m_value;
};

} // namespace network

} // namespace mixxx

Q_DECLARE_METATYPE(mixxx::network::RequestId);
