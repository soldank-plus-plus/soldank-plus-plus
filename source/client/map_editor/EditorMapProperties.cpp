module;

#include <string>

export module MapEditor.EditorMapProperties;

import MapEditorState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class EditorMapProperties
{
public:
    EditorMapProperties(MapEditorState& map_editor_state, StateManager& game_state_manager)
    {
        map_editor_state.event_save_map.AddObserver(
          [&game_state_manager](const std::string& map_name) {
              game_state_manager.GetMap().SaveMap(map_name);
          });
        map_editor_state.event_set_map_name.AddObserver(
          [&game_state_manager](const std::string& map_name) {
              game_state_manager.GetMap().SetName(map_name);
          });
        map_editor_state.event_set_map_description.AddObserver(
          [&game_state_manager](const std::string& description) {
              game_state_manager.GetMap().SetDescription(description);
          });
        map_editor_state.event_set_map_weather_type.AddObserver(
          [&game_state_manager](PMSWeatherType weather_type) {
              game_state_manager.GetMap().SetWeatherType(weather_type);
          });
        map_editor_state.event_set_map_step_type.AddObserver(
          [&game_state_manager](PMSStepType step_type) {
              game_state_manager.GetMap().SetStepType(step_type);
          });
        map_editor_state.event_set_map_grenades_count.AddObserver(
          [&game_state_manager](unsigned char grenades_count) {
              game_state_manager.GetMap().SetGrenadesCount(grenades_count);
          });
        map_editor_state.event_set_map_medikits_count.AddObserver(
          [&game_state_manager](unsigned char medikits_count) {
              game_state_manager.GetMap().SetMedikitsCount(medikits_count);
          });
        map_editor_state.event_set_map_jet_count.AddObserver([&game_state_manager](int jet_count) {
            game_state_manager.GetMap().SetJetCount(jet_count);
        });
        map_editor_state.event_set_map_background_top_color.AddObserver(
          [&game_state_manager](const PMSColor& background_top_color) {
              game_state_manager.GetMap().SetBackgroundTopColor(background_top_color);
          });
        map_editor_state.event_set_map_background_bottom_color.AddObserver(
          [&game_state_manager](const PMSColor& background_bottom_color) {
              game_state_manager.GetMap().SetBackgroundBottomColor(background_bottom_color);
          });
        map_editor_state.event_set_map_texture_name.AddObserver(
          [&game_state_manager](const std::string& texture_name) {
              game_state_manager.GetMap().SetTextureName(texture_name);
          });
    }
};
} // namespace Soldank
