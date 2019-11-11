#include "library/aoide/agent.h"

#include "library/aoide/subsystem.h"
#include "util/logger.h"

namespace mixxx {

namespace aoide {

namespace {

const Logger kLogger("aoide Agent");

} // anonymous namespace

Agent::Agent(
        Subsystem* subsystem,
        QObject* parent)
        : QObject(parent),
          m_subsystem(subsystem) {
}

void Agent::connectSlots() {
    connect(m_subsystem,
            &Subsystem::collectionsChanged,
            this,
            &Agent::onCollectionsChanged);
}

void Agent::onCollectionsChanged(
        int flags) {
    // Ensure that one collection is active. If no collections
    // exist then create a default one.
    switch (m_subsystem->allCollections().size()) {
    case 0: {
        DEBUG_ASSERT(!m_subsystem->hasActiveCollection());
        AoideCollection collection;
        collection.setName("Mixxx Collection");
        collection.setDescription("Created by Mixxx");
        m_subsystem->invokeCreateCollection(collection);
        break;
    }
    case 1: {
        if (!m_subsystem->hasActiveCollection()) {
            m_subsystem->selectActiveCollection(
                    m_subsystem->allCollections().front().header().uid());
        }
        break;
    }
    default:
        if (!m_subsystem->hasActiveCollection()) {
            kLogger.warning()
                    << "TODO: Choose one of the available collections";
            // Simply activate the first one
            m_subsystem->selectActiveCollection(
                    m_subsystem->allCollections().front().header().uid());
        }
        break;
    }
    if ((flags & Subsystem::CollectionsChangedFlags::ACTIVE_COLLECTION) && m_subsystem->hasActiveCollection()) {
        kLogger.info()
                << "Active collection"
                << m_subsystem->activeCollection();
    }
}

} // namespace aoide

} // namespace mixxx
