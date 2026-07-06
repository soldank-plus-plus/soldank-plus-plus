module;

#include <functional>
#include <span>
#include <vector>

export module Runtime.ServerSimulationEventRouter;

import Shared.Core.Simulation.SimulationEventSink;
import Shared.Core.Simulation.SimulationEvents;

export namespace Soldank
{
class ServerSimulationEventRouter : public SimulationEventSink
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
