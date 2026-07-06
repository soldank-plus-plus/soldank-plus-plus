module;

#include <span>

export module Shared.Core.Simulation.SimulationEventSink;

import Shared.Core.Simulation.SimulationEvents;

export namespace Soldank
{
class SimulationEventSink
{
public:
    virtual ~SimulationEventSink() = default;
    virtual void OnSimulationEvents(std::span<const SimulationEvent> events) = 0;
};
} // namespace Soldank
