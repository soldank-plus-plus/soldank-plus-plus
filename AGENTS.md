# AGENTS.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

C++23 reimplementation of the OpenSoldat game engine. Ships two executables — `client` (OpenGL/GLFW/ImGui front-end with an integrated map editor) and `server` (headless, with daScript scripting and an HTTP lobby client). Both link a shared library that owns the simulation (physics, animations, entities, networking protocol).

## Build

The project requires a C++23-capable compiler (G++ 13, Clang 15, or MSVC 17.6.5+) and CMake ≥ 3.28 (C++ modules are used pervasively, so a recent CMake + Ninja generator is recommended).

Vcpkg is required to pull `GameNetworkingSockets` (it can't come through CPM). The top-level `CMakeLists.txt` auto-detects `./vcpkg/scripts/buildsystems/vcpkg.cmake` and wires it as the toolchain file, so the standard flow is:

```
git clone https://github.com/microsoft/vcpkg && ./vcpkg/bootstrap-vcpkg.sh
cmake -Bbuild [-DCMAKE_EXPORT_COMPILE_COMMANDS=On]
cmake --build build -j
```

CMake options:
- `BUILD_CLIENT_ENABLED`, `BUILD_SERVER_ENABLED`, `BUILD_TESTS_ENABLED`.
- `BUILD_COMPILE_WITH_COVERAGE` defaults `OFF`; use the `clang-ninja-coverage` preset to enable gcov-compatible coverage flags (`-coverage -fprofile-arcs -ftest-coverage`).

Artifacts land in `build/bin/<Config>/` (POSIX) or `build/bin/` (MSVC); `install` copies them to `./bin/`.

CPM (`cmake/CPM.cmake`) downloads two external dependencies at configure time: `soldatbase` (game assets — animations under `shared/anims/*.poa`, `shared/objects/*.po`, weapon INIs) and `daScript` (server-only).

## Tests

GTest, driven by CTest. Each test is its own executable declared in `tests/shared/CMakeLists.txt`:

```
cd build && ctest --output-on-failure          # all tests
ctest -R MapTest                                # one test by name
./bin/Debug/MapTest --gtest_filter=Foo.Bar      # run a single GTest case directly
```

`MovementTest` is declared but its `add_test()` is commented out — it runs a full soldier-movement simulation against the `SoldierMovementSimulation` module and copies `.poa`/`.po`/`weapons*.ini` from `soldatbase` into its working directory via `POST_BUILD` steps. Run it directly from its build output dir, not from an arbitrary cwd.

Shared GTest helpers live in `tests/testing_frameworks/shared_lib_testing/` (linked as `shared_lib_testing_framework`).

## Architecture

Three CMake targets, all built as C++ module libraries (`FILE_SET ... TYPE CXX_MODULES`) — public interface is via `import`, not headers:

- **`shared_lib`** (`source/shared/`) — pure simulation. `Shared.Core.World` runs the fixed-timestep game loop; `IWorld` is the interface both client and server depend on. Subsystems: `core/physics/` (soldier/bullet/item/skeleton), `core/animations/` with a per-state class under `animations/states/`, `core/entities/`, `core/map/` (PMS map format), `core/state/` (StateManager owns mutable world state). `communication/` defines the wire protocol: `NetworkEvent` enum, `NetworkMessage`, `NetworkPackets`, and the `NetworkEventDispatcher` that fans incoming messages out to handlers.
- **`client_lib`** + `client` exe (`source/client/`) — adds `Application` (top-level lifecycle), GLFW `Window`, input, OpenGL `rendering/` (one renderer per drawable kind, plus ImGui-based debug/map-editor UI), `map_editor/` (command-pattern actions + tools), and `networking/` with a `NetworkingClient` that registers concrete `*NetworkEventHandler`s into the dispatcher.
- **`server_lib`** + `server` exe (`source/server/`) — `GameServer` drives the same `IWorld`; networking uses poll groups (`EntryPollGroup` for handshake, `PlayerPollGroup` once authenticated). Scripting goes through `IScriptingEngine` → `DaScriptScriptingEngine`. `LobbyClient` talks to a master server via cpp-httplib.

When adding a new networked message: extend the `NetworkEvent` enum in `source/shared/communication/NetworkEvent.cpp`, add packet structs in `NetworkPackets.cpp`, then add a handler under `source/client/networking/event_handlers/` and/or `source/server/networking/event_handlers/`, and register it where the dispatcher is constructed (`Application.cpp` on the client, `GameServer.cpp` on the server). All three source files must also be appended to the `*_lib_modules` list in the corresponding `CMakeLists.txt` — the module file set is explicit, not glob-based.

## Code style

`.clang-format` enforces Mozilla base, 4-space indent, 100-col, custom brace wrapping (`AfterClass/Function/Struct/Namespace = true`). `SortIncludes` is off — preserve the existing include order.

`.clang-tidy` enforces naming via `readability-identifier-naming`:
- Namespaces, classes, structs, functions, enum constants, template params: **CamelCase**
- Variables, parameters, members: **lower_case**
- Private/protected members and class members: **lower_case with trailing `_`**
- Constants, constexpr, macros: **UPPER_CASE**

Checks enabled: `bugprone-*, cppcoreguidelines-*, clang-analyzer-*, modernize-*, readability-*, misc-*, performance-*` with a small explicit deny-list (magic-numbers, identifier-length, trailing-return-type, nodiscard, easily-swappable-parameters, pro-type-union-access — the last so `glm::vec2::x` access doesn't fire).

