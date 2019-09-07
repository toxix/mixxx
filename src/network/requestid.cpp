#include "network/requestid.h"

#include <QAtomicInteger>


namespace mixxx {

namespace network {

namespace {

QAtomicInteger<RequestId::value_t> s_value(static_cast<RequestId::value_t>(RequestId()));

} // anonymous namespace

/*static*/ void RequestId::registerMetaType() {
    qRegisterMetaType<RequestId>("mixxx::network::RequestId");
}

RequestId RequestId::nextValid() {
    RequestId next;
    do {
        next = RequestId(++s_value);
    } while (!next.isValid());
    return next;
}

} // namespace network

} // namespace mixxx
