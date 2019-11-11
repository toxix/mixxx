#pragma once

#include <QPointer>

namespace mixxx {

namespace aoide {

class Subsystem;

// The agent is a bot that listens to collection changes and
// automatically activates one of the available collections.
// TODO: Ask the user for explicitly selecting an active
// collection if multiple collections are available.
class Agent : public QObject {
    Q_OBJECT

  public:
    explicit Agent(
            Subsystem* subsystem,
            QObject* parent = nullptr);
    ~Agent() override = default;

    // Connect slots after moveToThread()
    void connectSlots();

  private slots:
    void /*Subsystem*/ onCollectionsChanged(
            int flags);

  private:
    const QPointer<Subsystem> m_subsystem;
};

} // namespace aoide

} // namespace mixxx
