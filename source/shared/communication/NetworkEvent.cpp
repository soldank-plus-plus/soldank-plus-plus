export module Shared.Networking.NetworkEvent;

export namespace Soldank
{
enum class NetworkEvent : unsigned int
{
    ChatMessage = 0,
    AssignPlayerId,
    SpawnSoldier,
    SoldierInput,
    SoldierState,
    SoldierInfo,
    PlayerLeave,
    PingCheck,
    ProjectileSpawn,
    KillCommand,
    KillSoldier,
    HitSoldier
};
}
