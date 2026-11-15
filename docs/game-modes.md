# Game modes

Game rules are compiled C++23 modules under `source/shared/core/game_modes/`. The
preset definitions in `BuiltinGameModes.cpp` use method chaining to compose
independent rule factories. No OpenSoldat implementation is imported or linked.

## Running a mode

In the server's existing `soldat.ini`:

```ini
[GAME]
Game_Mode=capture_the_flag
Survival_Mode=false
Survival_Clear_Weapons=false
Friendly_Fire=false
Score_Limit=10
Time_Limit_Ticks=36000
Map=maps/ctf_Ash.pms
```

IDs: `deathmatch`, `pointmatch`, `teammatch`, `capture_the_flag`, `rambomatch`,
`infiltration`, `hold_the_flag`. `sandbox` preserves unrestricted editor and local
experimentation. A missing Score_Limit, or zero, uses the preset's limit. Zero
Time_Limit_Ticks disables the time limit. The C++ builder uses `std::nullopt` for
unlimited values and rejects numeric zero limits.

Optional `Map_Rotation=maps/ctf_Ash.pms,maps/ctf_Laos.pms` cycles maps after a
five-second results period. With no rotation, the configured map restarts. Local
competitive matches also restart their map after the results period. All
clients need the same local map assets. Map changes reset the match epoch,
objectives, projectiles, pending commands and scores. The existing asset system
does not download maps from a server.

Local selection: `client --local --map <map> --game-mode deathmatch`. Add
`--survival` for round elimination. Local defaults to `sandbox`; competitive
modes wait for opponents. Server joins automatically assign the least-populated
team. Team modes require the appropriate team spawns; objective modes require
matching active objective spawns. PM + Survival is rejected, reflecting the
missing reference round-ending rule rather than creating an undocumented mode.

## Adding a variant

A compiled config is ordinary C++ using the builder. Register it in your
composition root's `GameModeRegistry` and add a new module file to the explicit
CMake list if you create one:

```cpp
GameModeDefinition MakeDoublePoints()
{
    return GameModeBuilder("double_points")
      .DisplayName("Double Points")
      .AddRule(KillScoring({ .player_points = 2 }))
      .Respawn({ .delay = 120 })
      .ScoreLimit(60)
      .TimeLimit(36000)
      .Build();
}
```

`GameModeBuilder(existing_definition)` creates an independently configurable
copy. Definitions own immutable settings; each GameModeRuntime creates fresh
rule instances. There is no mode switch in physics or network handlers.

`ModeObjective` settings select an item type, owning team, fixed destination,
return duration, home radius and capture radius. `KillScoringSettings` controls
personal/team points, enemy eligibility, flag multipliers, multikills and special
bow scoring. Capture and periodic scoring are separate factories. `RoundPolicy`
controls elimination, capture termination, winner awards and reset delay.

For a new mechanic, implement `IGameRule`, then supply a stable RuleSpec ID and a
factory returning a fresh instance, or use `AddRule<YourRule>(id, settings)`.
Specify a deterministic settings hash and bump the rule version when its
semantics change. The resolved definition fingerprint includes registered rule
metadata and all built-in settings. IDs must be lowercase letters, digits and
underscores. Duplicate IDs and invalid timings fail validation.

Rules receive `KillFact`, `CaptureFact` or a read-only `ModeWorldView`. They append
typed score/capture/reset actions. A rule cannot access StateManager, networking,
rendering or another rule's private state. Runtime state belongs to the instance;
reset participant-local state in `ResetPlayer`. Fact/action capabilities are the
explicit extension boundary for mechanics needing new engine operations.

## Authority and timing

StateManager owns the match runtime alongside world state. WorldGameModeAdapter
converts physical state to facts, owns objective bindings and applies physical
resets. World commits each death once, retains lethal projectile attribution,
and asks the mode for respawn policy. Projectile owner generations prevent
credit reaching a different player after soldier-slot reuse.

Authoritative processing evaluates deaths, captures, periodic scores,
elimination and finally match limits. A capture ending a round suppresses its
elimination bonus. All valid scores in a tick commit before winner selection;
ties produce multiple winners. INF and HTF use the reference global modulo
timing, including population adjustments. Pointmatch preserves the previous
chain-count multiplier and timer-zero boundary. See the implementation plan for
the full reference rules catalog.

Online clients receive reliable bounded full snapshots on join, on semantic
changes and periodically. They accept newer epochs and tick/revision pairs,
reject incompatible versions and old snapshots, and never instantiate scoring
rules. A versioned first-message handshake rejects older clients before soldier
creation. This changes the network protocol; server and client must be updated
together. HUD scores and result messages read the authoritative match state.

Compile and test using `clang-ninja-debug-all`. GameModesTest exercises pure
rules; GameModeWorldTest covers physical integration; GameModeProtocolTest tests
late joins and stale/invalid snapshots.
