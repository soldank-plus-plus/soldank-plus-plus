module;

#include <optional>
#include <list>
#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

export module ClientState;

import Extern.Glm;

import Camera;
import MapEditorState;

import Shared.Networking.PingTimer;
import Shared.Networking.NetworkPackets;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.WeaponType;
import Shared.Core.Utility.Observable;

export namespace Soldank
{
struct ClientCameraState
{
    Camera view;
    glm::vec2 position;
    glm::vec2 previous_position;
    bool smooth = true;
};

struct ClientInputState
{
    glm::vec2 mouse_screen_position;
    glm::vec2 mouse_map_position;
    bool is_left_mouse_button_pressed;
    bool is_right_mouse_button_pressed;
    float window_width;
    float window_height;
};

struct ClientNetworkState
{
    glm::vec2 soldier_position_server_pov;

    struct PredictedSoldierSnapshot
    {
        std::uint32_t input_sequence_id;
        std::uint32_t client_tick;
        std::uint32_t apply_server_tick;
        SoldierSnapshot soldier_snapshot;
    };

    // Network delivery history is pruned by acknowledged input sequence ID.
    std::list<SoldierInputPacket> pending_inputs;
    // Simulation replay history is pruned independently by application server tick.
    std::list<SoldierInputPacket> prediction_inputs;
    std::list<PredictedSoldierSnapshot> soldier_snapshot_history;
    bool server_reconciliation = true;
    bool client_side_prediction = true;
    bool objects_interpolation = true;
    bool draw_server_pov_client_pos;

    std::optional<std::int64_t> server_tick_offset;
    std::uint32_t target_input_delay_ticks = 5;
    std::uint32_t active_input_delay_ticks = 0;
    std::uint32_t input_timeline_resync_count = 0;
    std::optional<std::uint32_t> reconciliation_resume_server_tick;

    int network_lag;

#ifndef NDEBUG
    float last_local_correction_distance = 0.0F;
    float maximum_local_correction_distance = 0.0F;
    unsigned int local_correction_count = 0;
#endif

    PingTimer ping_timer;
};

struct ClientDebugRenderState
{
    bool draw_colliding_polygons = false;
    bool draw_soldier_hitboxes = false;
    bool draw_bullet_hitboxes = false;
    bool draw_item_hitboxes = false;
    bool draw_sectors = false;
    bool draw_map_boundaries = false;
    std::vector<unsigned int> colliding_polygon_ids;

    bool is_game_debug_interface_enabled = false;
};

struct ClientWorldRenderOptions
{
    bool draw_background = true;
    bool draw_polygons = true;
    bool draw_sceneries = true;

    glm::vec2 current_polygon_texture_dimensions;
};

struct ClientState
{
    /**
     * This is nullopt when the soldier is not created yet.
     * For example when client is still in a connecting state
     * with the server and server didn't create the Soldier
     * for the client
     */
    std::optional<std::uint8_t> client_soldier_id;

    ClientCameraState camera;
    ClientInputState input;
    ClientNetworkState network;

    WeaponType primary_weapon_type_choice = WeaponType::DesertEagles;
    WeaponType secondary_weapon_type_choice = WeaponType::USSOCOM;

    bool kill_button_just_pressed = false;

    bool player_was_holding_left = false;
    bool player_was_holding_right = false;
    bool player_was_running_left = false;
    bool player_was_jumping = false;

    ClientDebugRenderState debug_render;

    MapEditorState map_editor_state;

    ClientWorldRenderOptions world_render_options;

    Observable<glm::vec2> event_window_resized;

    Observable<glm::vec2, glm::vec2> event_mouse_screen_position_changed;
    Observable<glm::vec2, glm::vec2> event_mouse_map_position_changed;
    Observable<> event_left_mouse_button_clicked;
    Observable<> event_left_mouse_button_released;
    Observable<> event_right_mouse_button_clicked;
    Observable<> event_right_mouse_button_released;
    Observable<> event_middle_mouse_button_clicked;
    Observable<> event_middle_mouse_button_released;
    Observable<> event_mouse_wheel_scrolled_up;
    Observable<> event_mouse_wheel_scrolled_down;
    Observable<int, int> event_key_pressed;
    Observable<int, int> event_key_released;

    Observable<unsigned int> event_respawn_soldier_requested;
    Observable<unsigned int> event_respawn_player_at_spawn_point_requested;
};
} // namespace Soldank
