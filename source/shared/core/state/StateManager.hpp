#ifndef __STATE_MANAGER_HPP__
#define __STATE_MANAGER_HPP__

#include "core/state/State.hpp"
#include "core/state/Control.hpp"

#include "core/types/ItemType.hpp"

#include <memory>
#include <functional>
#include <random>

namespace Soldank
{
class StateManager
{
public:
    StateManager(
      AnimationDataManager& animation_data_manager,
      std::shared_ptr<ParticleSystem> skeleton = ParticleSystem::Load(ParticleSystemType::Soldier));

    Map& GetMap() { return state_.map; } // TODO: Change this to const
    const Map& GetConstMap() const { return state_.map; }
    void OverrideMap(const Map& map) { state_.map = map; }

    void ChangeSoldierControlActionState(std::uint8_t soldier_id,
                                         ControlActionType control_action_type,
                                         bool new_state);
    void SoldierControlApply(
      std::uint8_t soldier_id,
      const std::function<void(const Soldier& soldier, Control& control)>& apply_function);

    void ChangeSoldierMouseMapPosition(std::uint8_t soldier_id, glm::ivec2 new_mouse_position);
    void SwitchSoldierWeapon(std::uint8_t soldier_id);
    void ChangeSoldierPrimaryWeapon(std::uint8_t soldier_id, WeaponType new_weapon_type);
    void SoldierPickupWeapon(std::uint8_t soldier_id, const Item& item);
    void SoldierPickupKit(std::uint8_t soldier_id, std::uint8_t item_id);
    void MoveSoldier(std::uint8_t soldier_id, const glm::vec2& move_offset);
    void SetSoldierPosition(std::uint8_t soldier_id, const glm::vec2& new_position);
    void ThrowSoldierFlags(std::uint8_t soldier_id);
    glm::vec2 GetSoldierAimDirection(std::uint8_t soldier_id);
    void TransformSoldier(std::uint8_t soldier_id,
                          const std::function<void(Soldier&)>& transform_soldier_function);
    void TransformSoldiers(const std::function<void(Soldier&)>& transform_soldier_function);
    const Soldier& GetSoldier(std::uint8_t soldier_id) const;
    const Soldier& CreateSoldier(std::optional<unsigned int> force_soldier_id);
    glm::vec2 SpawnSoldier(unsigned int soldier_id, std::optional<glm::vec2> spawn_position);
    void ForEachSoldier(
      const std::function<void(const Soldier& soldier)>& for_each_soldier_function) const;
    void ForSoldier(std::uint8_t soldier_id,
                    const std::function<void(const Soldier& soldier)>& for_soldier_function) const;
    const Soldier* FindSoldier(const std::function<bool(const Soldier& soldier)>& predicate) const;
    void RemoveSoldier(std::uint8_t soldier_id);
    std::size_t GetSoldiersCount() const;

    void EnqueueNewProjectile(const BulletParams& bullet_params);
    const Bullet* CreateProjectile(const BulletParams& bullet_params);
    const std::vector<BulletParams>& GetBulletEmitter() const;
    void ClearBulletEmitter();
    void ForEachBullet(
      const std::function<void(const Bullet& bullet)>& for_each_bullet_function) const;
    std::size_t GetBulletsCount() const;
    void TransformBullets(const std::function<void(Bullet& bullet)>& transform_bullet_function);
    void TransformBullet(std::uint8_t bullet_id,
                         const std::function<void(Bullet& bullet)>& transform_bullet_function);

    Item& CreateItem(glm::vec2 position, std::uint8_t owner_id, ItemType style);
    void SetItemPosition(unsigned int id, glm::vec2 new_position);
    void MoveItemIntoDirection(unsigned int id, glm::vec2 direction);
    void TransformItems(const std::function<void(Item& item)>& transform_item_function);
    void ForEachItem(const std::function<void(const Item& item)>& for_each_item_function) const;

    unsigned int GetGameTick() const { return state_.game_tick; }
    void SetGameTick(unsigned int new_game_tick) { state_.game_tick = new_game_tick; }

    bool IsGamePaused() const { return state_.paused; }
    void PauseGame() { state_.paused = true; }
    void UnPauseGame() { state_.paused = false; }
    void TogglePauseGame() { state_.paused = !state_.paused; }

private:
    Soldier& GetSoldierRef(std::uint8_t soldier_id);
    Item& GetItemRef(std::uint8_t item_id);

    State state_;
    std::vector<BulletParams> bullet_emitter_;

    std::random_device random_device_{};
    std::mt19937 mersenne_twister_engine_{ random_device_() };
};
} // namespace Soldank

#endif
