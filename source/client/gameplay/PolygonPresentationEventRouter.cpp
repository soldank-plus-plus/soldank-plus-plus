module;

#include <memory>
#include <deque>
#include <algorithm>
#include <functional>
#include <string_view>
#include <utility>

export module Gameplay.PolygonPresentationEventRouter;

import Audio.AudioEvents;
import ClientState;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.Polygons.PolygonContact;

export namespace Soldank
{
class PolygonPresentationEventRouter
{
public:
    static std::shared_ptr<PolygonPresentationEventRouter> Create(
      PhysicsEvents& physics,
      std::shared_ptr<AudioEvents> audio,
      std::shared_ptr<ClientState> client)
    {
        auto router = std::shared_ptr<PolygonPresentationEventRouter>(
          new PolygonPresentationEventRouter(std::move(audio), std::move(client)));
        physics.polygon_event.AddObserver([weak = std::weak_ptr<PolygonPresentationEventRouter>(
                                             router)](const PolygonEffectEvent& event) {
            if (const auto self = weak.lock();
                self && !self->client_->network.replaying_prediction) {
                self->Observe(event);
            }
        });
        return router;
    }

private:
    PolygonPresentationEventRouter(std::shared_ptr<AudioEvents> audio,
                                   std::shared_ptr<ClientState> client)
        : audio_(std::move(audio))
        , client_(std::move(client))
    {
    }
    void Observe(const PolygonEffectEvent& event)
    {
        if (std::ranges::any_of(seen_, [&](const auto& previous) {
                return previous.kind == event.kind && previous.soldier == event.soldier &&
                       previous.tick == event.tick && previous.life == event.life &&
                       previous.polygon == event.polygon && previous.ordinal == event.ordinal &&
                       previous.map_fingerprint == event.map_fingerprint;
            })) {
            return;
        }
        seen_.push_back(event);
        if (seen_.size() > 512) {
            seen_.pop_front();
        }
        std::string_view sound;
        switch (event.kind) {
            case PolygonEventKind::Hurt:
                sound = "arg";
                break;
            case PolygonEventKind::Heal:
                sound = "regenerate";
                break;
            case PolygonEventKind::Lava:
                sound = "lava";
                break;
            case PolygonEventKind::Bounce:
                if (event.amount > 1.0F) {
                    sound = "bounce";
                }
                break;
            case PolygonEventKind::Landing:
                sound = event.amount > 3.5F ? "fall-hard" : "fall";
                break;
            case PolygonEventKind::Death:
                sound = event.severity == DeathSeverity::Brutal ? "death3" : "death";
                break;
        }
        if (!sound.empty()) {
            audio_->sound_requested.Notify({ .command = AudioCommand::Play, .sound = sound });
        }
        if (client_->polygon_visual_events.size() >= 512) {
            client_->polygon_visual_events.erase(client_->polygon_visual_events.begin());
        }
        client_->polygon_visual_events.push_back(event);
    }
    std::shared_ptr<AudioEvents> audio_;
    std::shared_ptr<ClientState> client_;
    std::deque<PolygonEffectEvent> seen_;
};
} // namespace Soldank
