#ifndef __CLIENT_STATE_HPP__
#define __CLIENT_STATE_HPP__

#include "rendering/renderer/interface/map_editor/MapEditorState.hpp"
#include "rendering/components/Camera.hpp"

#include "core/math/Glm.hpp"

#include "communication/NetworkPackets.hpp"
#include "communication/PingTimer.hpp"

#include <optional>
#include <list>
#include <chrono>
#include <cstdint>

namespace Soldank
{
struct ClientState
{
    /**
     * This is nullopt when the soldier is not created yet.
     * For example when client is still in a connecting state
     * with the server and server didn't create the Soldier
     * for the client
     */
    std::optional<std::uint8_t> client_soldier_id;

    Camera camera_component; // TODO: change name to just camera
    glm::vec2 camera;        // TODO: change name to camera_position or move it to the Camera class
    glm::vec2 camera_prev;
    bool smooth_camera = true;
    glm::vec2 mouse;
    bool mouse_left_button_pressed;
    bool mouse_right_button_pressed;
    float game_width;
    float game_height;
    float window_width;
    float window_height;

    glm::vec2 soldier_position_server_pov;

    std::list<SoldierInputPacket> pending_inputs;
    bool server_reconciliation;
    bool client_side_prediction;
    bool objects_interpolation;
    bool draw_server_pov_client_pos;

    int network_lag;

    PingTimer ping_timer;

    WeaponType primary_weapon_type_choice = WeaponType::DesertEagles;
    WeaponType secondary_weapon_type_choice = WeaponType::USSOCOM;

    bool kill_button_just_pressed = false;

    bool player_was_holding_left = false;
    bool player_was_holding_right = false;
    bool player_was_running_left = false;
    bool player_was_jumping = false;

    bool draw_colliding_polygons = false;
    bool draw_soldier_hitboxes = false;
    bool draw_bullet_hitboxes = false;
    bool draw_item_hitboxes = false;
    std::vector<unsigned int> colliding_polygon_ids;

    bool draw_map_editor_interface = false;
    bool draw_game_debug_interface = false;
    bool draw_game_interface = false;

    MapEditorState map_editor_state;

    bool draw_background = true;
    bool draw_polygons = true;
    bool draw_sceneries = true;

    glm::vec2 current_polygon_texture_dimensions;

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
    Observable<int> event_key_pressed;
    Observable<int> event_key_released;
};
} // namespace Soldank

#endif
