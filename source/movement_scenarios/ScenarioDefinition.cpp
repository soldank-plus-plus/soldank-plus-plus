module;
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
export module MovementScenarios.Definition;
export import MovementScenarios.Id;
export import Extern.Glm;
export import Shared.Core.Map.PMSStructs;
export import Shared.Core.Map.PMSEnums;
export import Shared.Core.State.Control;
export import Shared.Core.Entities.Soldier;
export import Shared.Core.Types.TeamType;
export import Shared.Core.Types.WeaponType;
export namespace Soldank::MovementScenarios
{
struct Input
{
    bool left{}, right{}, jump{}, jets{}, crouch{}, prone{};
};
struct InputChange
{
    std::uint32_t tick{};
    ControlActionType action{};
    bool held{};
};
struct AimChange
{
    std::uint32_t tick{};
    glm::vec2 aim{};
    bool relative = true;
};
struct ScenarioDefinition
{
    ScenarioId id = ScenarioId::IdleLeft;
    std::string description;
    std::vector<PMSPolygon> polygons;
    std::string texture = "default.png";
    glm::vec2 spawn{ 0, -34 };
    std::uint8_t soldier_id{};
    SoldierAppearance appearance;
    TeamType team = TeamType::None;
    WeaponType primary = WeaponType::DesertEagles;
    WeaponType secondary = WeaponType::USSOCOM;
    PMSColor background_top{ 100, 150, 180, 255 };
    PMSColor background_bottom{ 30, 40, 50, 255 };
    glm::vec2 camera_position{};
    bool follow_camera = true;
    float camera_zoom = 1.0F;
    float inspection_margin = 2000.0F;
    glm::vec2 aim{ 6400, 0 };
    bool relative_aim = true;
    glm::vec2 initial_velocity{};
    int fuel = 240;
    int jet_capacity = 240;
    std::uint32_t duration_ticks = 180;
    std::uint32_t settle_ticks = 5;
    std::uint32_t settle_limit = 5000;
    Input initial_input;
    std::uint32_t preparation_ticks{};
    std::vector<InputChange> preparation_changes;
    std::vector<InputChange> changes;
    std::vector<AimChange> aim_changes;
};
void Validate(const ScenarioDefinition& definition)
{
    if (!definition.appearance.IsValid() || definition.team >= TeamType::Spectator ||
        !IsPrimaryWeaponChoice(definition.primary) ||
        !IsSecondaryWeaponChoice(definition.secondary) || !std::isfinite(definition.camera_zoom) ||
        definition.camera_zoom <= 0 || !std::isfinite(definition.inspection_margin) ||
        definition.inspection_margin < 0 || definition.soldier_id >= 32) {
        throw std::invalid_argument("Invalid soldier or presentation settings");
    }
    if (definition.id < ScenarioId::IdleLeft || definition.id >= ScenarioId::Count ||
        definition.polygons.empty() || definition.duration_ticks == 0 ||
        definition.duration_ticks > 1000000 || definition.settle_limit > 1000000 ||
        definition.preparation_ticks > 1000000 ||
        definition.settle_ticks > definition.settle_limit || definition.fuel < 0 ||
        definition.jet_capacity < definition.fuel) {
        throw std::invalid_argument("Invalid scenario definition");
    }
    auto finite = [](glm::vec2 value) { return std::isfinite(value.x) && std::isfinite(value.y); };
    if (!finite(definition.spawn) || !finite(definition.aim) ||
        !finite(definition.initial_velocity) || !finite(definition.camera_position)) {
        throw std::invalid_argument("Non-finite scenario coordinates");
    }
    for (const auto& polygon : definition.polygons) {
        if (polygon.polygon_type > PMSPolygonType::BackgroundTransition ||
            !std::isfinite(polygon.bounciness) || polygon.bounciness < 0) {
            throw std::invalid_argument("Invalid polygon attributes");
        }
        for (const auto& vertex : polygon.vertices) {
            if (!std::isfinite(vertex.z) || !std::isfinite(vertex.rhw) ||
                !std::isfinite(vertex.texture_s) || !std::isfinite(vertex.texture_t)) {
                throw std::invalid_argument("Non-finite polygon vertex attributes");
            }
        }
        const auto& a = polygon.vertices[0];
        const auto& b = polygon.vertices[1];
        const auto& c = polygon.vertices[2];
        if (!finite({ a.x, a.y }) || !finite({ b.x, b.y }) || !finite({ c.x, c.y }) ||
            std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) < 0.001F) {
            throw std::invalid_argument("Invalid or degenerate polygon");
        }
    }
    std::set<std::uint32_t> aim_ticks;
    for (const auto& aim : definition.aim_changes) {
        if (aim.tick >= definition.duration_ticks || !finite(aim.aim) ||
            !aim_ticks.insert(aim.tick).second) {
            throw std::invalid_argument("Invalid aim change");
        }
    }
    std::map<std::pair<std::uint32_t, ControlActionType>, bool> assigned;
    for (const auto& change : definition.changes) {
        if (change.tick >= definition.duration_ticks) {
            throw std::invalid_argument("Timeline tick out of range");
        }
    }
    auto all_changes = definition.changes;
    for (auto change : definition.preparation_changes) {
        if (change.tick >= definition.preparation_ticks) {
            throw std::invalid_argument("Preparation tick out of range");
        }
        change.tick += definition.duration_ticks;
        all_changes.push_back(change);
    }
    for (const auto& change : all_changes) {
        if (change.tick >= definition.duration_ticks + definition.preparation_ticks ||
            (change.action != ControlActionType::MoveLeft &&
             change.action != ControlActionType::MoveRight &&
             change.action != ControlActionType::Jump &&
             change.action != ControlActionType::UseJets &&
             change.action != ControlActionType::Crouch &&
             change.action != ControlActionType::Prone)) {
            throw std::invalid_argument("Invalid timeline input");
        }
        auto [entry, inserted] =
          assigned.emplace(std::pair{ change.tick, change.action }, change.held);
        if (!inserted && entry->second != change.held) {
            throw std::invalid_argument("Conflicting inputs at the same tick");
        }
    }
}
class ScenarioBuilder
{
public:
    explicit ScenarioBuilder(ScenarioId id) { definition_.id = id; }
    ScenarioBuilder& AddPolygon(glm::vec2 a,
                                glm::vec2 b,
                                glm::vec2 c,
                                PMSPolygonType type = PMSPolygonType::Normal)
    {
        PMSPolygon polygon{};
        const std::array vertices{ a, b, c };
        for (unsigned int i = 0; i < 3; ++i) {
            polygon.vertices[i].x = vertices[i].x;
            polygon.vertices[i].y = vertices[i].y;
            polygon.vertices[i].texture_s = vertices[i].x / 128.0F;
            polygon.vertices[i].texture_t = vertices[i].y / 128.0F;
            polygon.vertices[i].rhw = 1;
        }
        polygon.polygon_type = type;
        polygon.bounciness = 1;
        return AddPolygon(polygon);
    }
    ScenarioBuilder& AddPolygon(const PMSPolygon& polygon)
    {
        definition_.polygons.push_back(polygon);
        return *this;
    }
    ScenarioBuilder& SpawnSoldier(glm::vec2 position)
    {
        definition_.spawn = position;
        return *this;
    }
    ScenarioBuilder& AimRelativeToSoldier(glm::vec2 aim)
    {
        definition_.aim = aim;
        definition_.relative_aim = true;
        return *this;
    }
    ScenarioBuilder& SetDurationTicks(std::uint32_t ticks)
    {
        definition_.duration_ticks = ticks;
        return *this;
    }
    ScenarioBuilder& SettleBeforePlayback(std::uint32_t ticks, std::uint32_t limit)
    {
        definition_.settle_ticks = ticks;
        definition_.settle_limit = limit;
        return *this;
    }
    ScenarioBuilder& ChangeAt(std::uint32_t tick, ControlActionType action, bool held)
    {
        definition_.changes.push_back({ tick, action, held });
        return *this;
    }
    ScenarioBuilder& PrepareFor(std::uint32_t ticks)
    {
        definition_.preparation_ticks = ticks;
        return *this;
    }
    ScenarioBuilder& PrepareChangeAt(std::uint32_t tick, ControlActionType action, bool held)
    {
        definition_.preparation_changes.push_back({ tick, action, held });
        return *this;
    }
    ScenarioBuilder& AimAt(std::uint32_t tick, glm::vec2 aim, bool relative = true)
    {
        definition_.aim_changes.push_back({ tick, aim, relative });
        return *this;
    }
    ScenarioDefinition Build() const
    {
        Validate(definition_);
        return definition_;
    }

private:
    ScenarioDefinition definition_;
};
} // namespace Soldank::MovementScenarios
