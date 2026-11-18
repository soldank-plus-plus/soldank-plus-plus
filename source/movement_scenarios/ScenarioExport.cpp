module;
#include <json.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
export module MovementScenarios.Export;
export import MovementScenarios.Runner;
namespace Soldank::MovementScenarios
{
using Json = nlohmann::json;
Json Vector(glm::vec2 value)
{
    return Json::array({ value.x, value.y });
}
glm::vec2 ReadVector(const Json& value)
{
    return { value.at(0).get<float>(), value.at(1).get<float>() };
}
Json Color(PMSColor color)
{
    return { color.red, color.green, color.blue, color.alpha };
}
PMSColor ReadColor(const Json& value)
{
    return { value.at(0), value.at(1), value.at(2), value.at(3) };
}
Json ColorVector(glm::vec4 value)
{
    return { value.x, value.y, value.z, value.w };
}
glm::vec4 ReadColorVector(const Json& value)
{
    return { value.at(0), value.at(1), value.at(2), value.at(3) };
}
Json DefinitionJson(const ScenarioDefinition& definition)
{
    Json polygons = Json::array(), changes = Json::array();
    for (const auto& polygon : definition.polygons) {
        Json vertices = Json::array();
        for (const auto& vertex : polygon.vertices) {
            vertices.push_back({ vertex.x,
                                 vertex.y,
                                 vertex.z,
                                 vertex.rhw,
                                 vertex.texture_s,
                                 vertex.texture_t,
                                 vertex.color.red,
                                 vertex.color.green,
                                 vertex.color.blue,
                                 vertex.color.alpha });
        }
        polygons.push_back({ { "vertices", vertices },
                             { "type", static_cast<int>(polygon.polygon_type) },
                             { "bounciness", polygon.bounciness } });
    }
    for (const auto& change : definition.changes) {
        changes.push_back({ change.tick, static_cast<int>(change.action), change.held });
    }
    Json preparation = Json::array(), aims = Json::array();
    for (const auto& change : definition.preparation_changes) {
        preparation.push_back({ change.tick, static_cast<int>(change.action), change.held });
    }
    for (const auto& change : definition.aim_changes) {
        aims.push_back({ change.tick, Vector(change.aim), change.relative });
    }
    const auto& initial = definition.initial_input;
    const auto& appearance = definition.appearance;
    Json appearance_json = { { "hair_style", static_cast<int>(appearance.hair_style) },
                             { "headgear", appearance.headgear },
                             { "chain", appearance.chain },
                             { "cigar", appearance.cigar },
                             { "shirt", ColorVector(appearance.shirt) },
                             { "pants", ColorVector(appearance.pants) },
                             { "hair", ColorVector(appearance.hair) },
                             { "skin", ColorVector(appearance.skin) },
                             { "jet", ColorVector(appearance.jet) } };
    return { { "soldier_id", definition.soldier_id },
             { "appearance", appearance_json },
             { "team", static_cast<int>(definition.team) },
             { "primary", static_cast<int>(definition.primary) },
             { "secondary", static_cast<int>(definition.secondary) },
             { "background_top", Color(definition.background_top) },
             { "background_bottom", Color(definition.background_bottom) },
             { "camera_position", Vector(definition.camera_position) },
             { "camera_zoom", definition.camera_zoom },
             { "follow_camera", definition.follow_camera },
             { "inspection_margin", definition.inspection_margin },
             { "preparation_ticks", definition.preparation_ticks },
             { "preparation_changes", preparation },
             { "aim_changes", aims },
             { "initial_input",
               { initial.left,
                 initial.right,
                 initial.jump,
                 initial.jets,
                 initial.crouch,
                 initial.prone } },
             { "id", ScenarioIdToString(definition.id) },
             { "description", definition.description },
             { "polygons", polygons },
             { "texture", definition.texture },
             { "spawn", Vector(definition.spawn) },
             { "aim", Vector(definition.aim) },
             { "relative_aim", definition.relative_aim },
             { "initial_velocity", Vector(definition.initial_velocity) },
             { "fuel", definition.fuel },
             { "jet_capacity", definition.jet_capacity },
             { "duration_ticks", definition.duration_ticks },
             { "settle_ticks", definition.settle_ticks },
             { "settle_limit", definition.settle_limit },
             { "changes", changes } };
}
std::string Fingerprint(const Json& definition)
{
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char character : definition.dump()) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << std::hex << hash;
    return output.str();
}
Json SampleJson(const Sample& sample)
{
    return { { "tick", sample.tick },
             { "input",
               { sample.input.left,
                 sample.input.right,
                 sample.input.jump,
                 sample.input.jets,
                 sample.input.crouch,
                 sample.input.prone } },
             { "aim", Vector(sample.aim) },
             { "position", Vector(sample.position) },
             { "displacement", Vector(sample.displacement) },
             { "velocity", Vector(sample.velocity) },
             { "grounded", sample.grounded },
             { "direction", sample.direction },
             { "fuel", sample.fuel },
             { "body_type", sample.body_type },
             { "legs_type", sample.legs_type },
             { "body_frame", sample.body_frame },
             { "legs_frame", sample.legs_frame },
             { "body_speed", sample.body_speed },
             { "legs_speed", sample.legs_speed } };
}
} // namespace Soldank::MovementScenarios
export namespace Soldank::MovementScenarios
{
void ExportScenario(const ScenarioRunner& runner, const std::string& path)
{
    const auto definition = DefinitionJson(runner.GetDefinition());
    Json samples = Json::array();
    for (const auto& sample : runner.GetTrace()) {
        samples.push_back(SampleJson(sample));
    }
    Json bundle = { { "schema_version", 1 },
                    { "definition", definition },
                    { "fingerprint", Fingerprint(definition) },
                    { "complete", runner.Completed() },
                    { "setup_ticks", runner.GetSetupTicks() },
                    { "coordinate_offset", Vector(runner.GetOffset()) },
                    { "origin", Vector(runner.GetOrigin()) },
                    { "starting_velocity", Vector(runner.GetStartingVelocity()) },
                    { "starting_fuel", runner.GetStartingFuel() },
                    { "starting_direction", runner.GetStartingDirection() },
                    { "simulation", "World::Tick/60Hz" },
                    { "build", MOVEMENT_SCENARIO_BUILD_ID },
                    { "samples", samples } };
    std::ofstream output(path);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << bundle.dump(2) << '\n';
}
ScenarioDefinition LoadScenario(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open scenario: " + path);
    }
    Json bundle;
    input >> bundle;
    if (bundle.at("schema_version") != 1) {
        throw std::invalid_argument("Unsupported scenario schema");
    }
    const auto& value = bundle.at("definition");
    if (bundle.at("fingerprint") != Fingerprint(value)) {
        throw std::invalid_argument("Scenario fingerprint mismatch");
    }
    ScenarioDefinition definition;
    definition.id = ScenarioIdFromString(value.at("id").get<std::string>());
    definition.description = value.at("description");
    definition.texture = value.at("texture");
    definition.spawn = ReadVector(value.at("spawn"));
    definition.aim = ReadVector(value.at("aim"));
    definition.relative_aim = value.at("relative_aim");
    definition.initial_velocity = ReadVector(value.at("initial_velocity"));
    definition.fuel = value.at("fuel");
    definition.jet_capacity = value.at("jet_capacity");
    definition.duration_ticks = value.at("duration_ticks");
    definition.settle_ticks = value.at("settle_ticks");
    definition.settle_limit = value.at("settle_limit");
    for (const auto& entry : value.at("polygons")) {
        PMSPolygon polygon{};
        polygon.polygon_type = static_cast<PMSPolygonType>(entry.at("type").get<int>());
        polygon.bounciness = entry.at("bounciness");
        if (entry.at("vertices").size() != 3) {
            throw std::invalid_argument("Polygon needs three vertices");
        }
        for (unsigned int i = 0; i < 3; ++i) {
            const auto& v = entry.at("vertices").at(i);
            auto& vertex = polygon.vertices[i];
            vertex.x = v.at(0);
            vertex.y = v.at(1);
            vertex.z = v.at(2);
            vertex.rhw = v.at(3);
            vertex.texture_s = v.at(4);
            vertex.texture_t = v.at(5);
            vertex.color = PMSColor(v.at(6), v.at(7), v.at(8), v.at(9));
        }
        definition.polygons.push_back(polygon);
    }
    for (const auto& entry : value.at("changes")) {
        definition.changes.push_back({ entry.at(0).get<unsigned int>(),
                                       static_cast<ControlActionType>(entry.at(1).get<int>()),
                                       entry.at(2).get<bool>() });
    }
    definition.soldier_id = value.at("soldier_id");
    definition.team = static_cast<TeamType>(value.at("team").get<unsigned int>());
    definition.primary = static_cast<WeaponType>(value.at("primary").get<int>());
    definition.secondary = static_cast<WeaponType>(value.at("secondary").get<int>());
    definition.background_top = ReadColor(value.at("background_top"));
    definition.background_bottom = ReadColor(value.at("background_bottom"));
    definition.camera_position = ReadVector(value.at("camera_position"));
    definition.camera_zoom = value.at("camera_zoom");
    definition.follow_camera = value.at("follow_camera");
    definition.inspection_margin = value.at("inspection_margin");
    const auto& appearance = value.at("appearance");
    definition.appearance.hair_style =
      static_cast<HairStyle>(appearance.at("hair_style").get<int>());
    definition.appearance.headgear = appearance.at("headgear");
    definition.appearance.chain = appearance.at("chain");
    definition.appearance.cigar = appearance.at("cigar");
    definition.appearance.shirt = ReadColorVector(appearance.at("shirt"));
    definition.appearance.pants = ReadColorVector(appearance.at("pants"));
    definition.appearance.hair = ReadColorVector(appearance.at("hair"));
    definition.appearance.skin = ReadColorVector(appearance.at("skin"));
    definition.appearance.jet = ReadColorVector(appearance.at("jet"));
    definition.preparation_ticks = value.at("preparation_ticks");
    const auto& initial = value.at("initial_input");
    definition.initial_input = { initial.at(0), initial.at(1), initial.at(2),
                                 initial.at(3), initial.at(4), initial.at(5) };
    for (const auto& entry : value.at("preparation_changes")) {
        definition.preparation_changes.push_back(
          { entry.at(0).get<unsigned int>(),
            static_cast<ControlActionType>(entry.at(1).get<int>()),
            entry.at(2).get<bool>() });
    }
    for (const auto& entry : value.at("aim_changes")) {
        definition.aim_changes.push_back(
          { entry.at(0).get<unsigned int>(), ReadVector(entry.at(1)), entry.at(2).get<bool>() });
    }
    Validate(definition);
    return definition;
}
void ExportCsv(const ScenarioRunner& runner, const std::string& path)
{
    std::ofstream output(path);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << std::setprecision(9);
    output << "tick,x,y,vx,vy,grounded,fuel,body_type,body_frame,legs_type,legs_frame\n";
    for (const auto& s : runner.GetTrace()) {
        output << s.tick << ',' << s.position.x << ',' << s.position.y << ',' << s.velocity.x << ','
               << s.velocity.y << ',' << s.grounded << ',' << s.fuel << ',' << s.body_type << ','
               << s.body_frame << ',' << s.legs_type << ',' << s.legs_frame << '\n';
    }
}
// Compare recorded observations exactly within one build. Tests choose reviewed
// checkpoints and numeric tolerances separately rather than approving recordings.
bool SameTrace(const ScenarioRunner& first, const ScenarioRunner& second)
{
    if (first.GetTrace().size() != second.GetTrace().size()) {
        return false;
    }
    for (unsigned int i = 0; i < first.GetTrace().size(); ++i) {
        if (SampleJson(first.GetTrace()[i]) != SampleJson(second.GetTrace()[i])) {
            return false;
        }
    }
    return true;
}
} // namespace Soldank::MovementScenarios
