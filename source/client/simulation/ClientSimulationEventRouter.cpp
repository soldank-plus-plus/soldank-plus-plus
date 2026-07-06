module;

#include <functional>
#include <span>
#include <vector>

export module Simulation.ClientSimulationEventRouter;

import Shared.Core.Simulation.SimulationEvents;
import Shared.Core.Simulation.SimulationEventSink;

export namespace Soldank
{
class ClientSimulationEventRouter : public SimulationEventSink
{
public:
    void AddSink(SimulationEventSink& sink) { sinks_.push_back(std::ref(sink)); }

    void OnSimulationEvents(std::span<const SimulationEvent> events) override
    {
        for (SimulationEventSink& sink : sinks_) {
            sink.OnSimulationEvents(events);
        }
    }

private:
    std::vector<std::reference_wrapper<SimulationEventSink>> sinks_;
};
} // namespace Soldank
