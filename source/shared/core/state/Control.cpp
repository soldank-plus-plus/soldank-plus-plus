export module Shared.Core.State.Control;

export namespace Soldank
{
enum class ControlActionType
{
    MoveLeft = 0,
    MoveRight,
    Jump,
    Crouch,
    Fire,
    UseJets,
    ChangeWeapon,
    ThrowGrenade,
    DropWeapon,
    Reload,
    Prone,
    ThrowFlag
};

struct Control
{
    bool left;
    bool right;
    bool up;
    bool down;
    bool fire;
    bool jets;
    bool change;
    bool throw_grenade;
    bool drop;
    bool reload;
    bool prone;
    bool flag_throw;
    int mouse_aim_x;
    int mouse_aim_y;
    int mouse_dist;
    bool was_running_left;
    bool was_jumping;
    bool was_throwing_weapon;
    bool was_changing_weapon;
    bool was_throwing_grenade;
    bool was_reloading_weapon;
};
} // namespace Soldank
