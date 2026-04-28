export module Shared.Core.Map.PMSEnums;

export namespace Soldank
{
enum class PMSPolygonType : unsigned char
{
    Normal = 0,
    OnlyBulletsCollide,
    OnlyPlayersCollide,
    NoCollide,
    Ice,
    Deadly,
    BloodyDeadly,
    Hurts,
    Regenerates,
    Lava,
    AlphaBullets,
    AlphaPlayers,
    BravoBullets,
    BravoPlayers,
    CharlieBullets,
    CharliePlayers,
    DeltaBullets,
    DeltaPlayers,
    Bouncy,
    Explosive,
    HurtFlaggers,
    FlaggerCollides,
    NonFlaggerCollides,
    FlagCollides
};

enum class PMSSceneryLevel : unsigned char
{
    BehindAll = 0,
    BehindMap,
    BehindNone
};

enum class PMSSpawnPointType : unsigned int
{
    General = 0,
    Alpha,
    Bravo,
    Charlie,
    Delta,
    AlphaFlag,
    BravoFlag,
    Grenades,
    Medkits,
    Clusters,
    Vest,
    Flamer,
    Berserker,
    Predator,
    YellowFlag,
    RamboBow,
    StatGun
};

enum class PMSSpecialAction : unsigned char
{
    None = 0,
    StopAndCamp,
    Wait1Second,
    Wait5Seconds,
    Wait10Seconds,
    Wait15Seconds,
    Wait20Seconds
};

enum class PMSStepType : unsigned char
{
    HardGround = 0,
    SoftGround,
    None
};

enum class PMSWeatherType : unsigned char
{
    None = 0,
    Rain,
    Sandstorm,
    Snow
};
} // namespace Soldank
