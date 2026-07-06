set(client_lib_modules
    stb.cpp

    application/Application.cpp
    application/ClientModes.cpp
    application/cli/CommandLineParameters.cpp
    application/window/Window.cpp
    application/window/WindowBackendAdapter.cpp
    application/input/InputRouter.cpp
    application/input/InputSnapshot.cpp
    application/input/PlatformInput.cpp
    application/input/PlayerInput.cpp
    application/platform/ClientTransportStartup.cpp
    application/platform/WebAssemblyStartupAdapter.cpp

    editor/EditorSession.cpp
    editor/PlayTestSession.cpp
    gameplay/GameSession.cpp

    map_editor/MapEditor.cpp
    map_editor/actions/MapEditorAction.cpp
    map_editor/actions/AddObjectsMapEditorAction.cpp
    map_editor/actions/AddPolygonMapEditorAction.cpp
    map_editor/actions/AddSceneryMapEditorAction.cpp
    map_editor/actions/AddSpawnPointMapEditorAction.cpp
    map_editor/actions/ColorObjectsMapEditorAction.cpp
    map_editor/actions/MoveSelectionMapEditorAction.cpp
    map_editor/actions/RemoveSelectionMapEditorAction.cpp
    map_editor/actions/RotateSelectionMapEditorAction.cpp
    map_editor/actions/ScaleSelectionMapEditorAction.cpp
    map_editor/actions/TransformPolygonsMapEditorAction.cpp
    map_editor/actions/TransformSceneriesMapEditorAction.cpp
    map_editor/actions/TransformSpawnPointsMapEditorAction.cpp
    map_editor/tools/ColorPickerTool.cpp
    map_editor/tools/ColorTool.cpp
    map_editor/tools/PolygonTool.cpp
    map_editor/tools/SceneryTool.cpp
    map_editor/tools/SelectionTool.cpp
    map_editor/tools/SpawnpointTool.cpp
    map_editor/tools/TextureTool.cpp
    map_editor/tools/Tool.cpp
    map_editor/tools/TransformTool.cpp
    map_editor/tools/VertexColorTool.cpp
    map_editor/tools/VertexSelectionTool.cpp
    map_editor/tools/WaypointTool.cpp

    networking/IConnection.cpp
    networking/NetworkingClient.cpp
    networking/INetworkingClient.cpp
    networking/NetworkClientSession.cpp
    networking/transport/ClientTransportAdapter.cpp
    networking/event_handlers/AssignPlayerIdNetworkEventHandler.cpp
    networking/event_handlers/HitSoldierNetworkEventHandler.cpp
    networking/event_handlers/KillSoldierNetworkEventHandler.cpp
    networking/event_handlers/PingCheckNetworkEventHandler.cpp
    networking/event_handlers/PlayerLeaveNetworkEventHandler.cpp
    networking/event_handlers/ProjectileSpawnNetworkEventHandler.cpp
    networking/event_handlers/SoldierInfoNetworkEventHandler.cpp
    networking/event_handlers/SoldierStateNetworkEventHandler.cpp
    networking/event_handlers/SpawnSoldierNetworkEventHandler.cpp

    runtime/ClientRuntime.cpp
    simulation/ClientSimulationEventRouter.cpp

    rendering/Scene.cpp
    rendering/ClientState.cpp
    rendering/components/Camera.cpp
    rendering/data/Texture.cpp
    rendering/data/sprites/SoldierPartData.cpp
    rendering/data/sprites/SpritesManager.cpp
    rendering/renderer/BackgroundRenderer.cpp
    rendering/renderer/BulletRenderer.cpp
    rendering/renderer/CircleRenderer.cpp
    rendering/renderer/ItemRenderer.cpp
    rendering/renderer/LineRenderer.cpp
    rendering/renderer/PolygonOutlinesRenderer.cpp
    rendering/renderer/PolygonsRenderer.cpp
    rendering/renderer/RectangleRenderer.cpp
    rendering/renderer/Renderer.cpp
    rendering/renderer/SceneriesRenderer.cpp
    rendering/renderer/SceneryOutlinesRenderer.cpp
    rendering/renderer/SoldierRenderer.cpp
    rendering/renderer/interface/CursorRenderer.cpp
    rendering/renderer/interface/ImGuiThemes.cpp
    rendering/renderer/interface/debug/DebugUI.cpp
    rendering/renderer/interface/map_editor/GridRenderer.cpp
    rendering/renderer/interface/map_editor/MapEditorScene.cpp
    rendering/renderer/interface/map_editor/MapEditorState.cpp
    rendering/renderer/interface/map_editor/MapEditorToolDetailsWindow.cpp
    rendering/renderer/interface/map_editor/MapEditorUI.cpp
    rendering/renderer/interface/map_editor/PolygonVertexOutlinesRenderer.cpp
    rendering/renderer/interface/map_editor/SingleImageRenderer.cpp
    rendering/renderer/interface/map_editor/SpawnPointRenderer.cpp
    rendering/renderer/TextRenderer.cpp
    rendering/shaders/Shader.cpp
)

if (BUILD_WEBASM_CLIENT_TRANSPORT)
    list(APPEND client_lib_modules
        networking/browser/WebRtcBrowserConnection.cpp
    )
else()
    list(APPEND client_lib_modules
        networking/Connection.cpp
        networking/interface/NetworkingInterface.cpp
    )
endif()

add_library(client_lib)

target_sources(client_lib
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        ${client_lib_modules}
)

target_compile_definitions(client_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS})
if (BUILD_WEBASM_CLIENT_TRANSPORT)
    target_compile_definitions(client_lib PUBLIC SOLDANK_WEBASM_CLIENT_TRANSPORT)
endif()

if(MSVC)
  target_compile_options(client_lib PRIVATE /W4)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(client_lib PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
target_include_directories(client_lib PRIVATE ${Stb_INCLUDE_DIR})
if (BUILD_WEBASM_CLIENT_TRANSPORT AND CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_include_directories(
    client_lib BEFORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/emscripten
  )
endif()
if (BUILD_WEBASM_CLIENT_TRANSPORT AND CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  find_path(EMSCRIPTEN_GLFW_INCLUDE_DIR
    NAMES GLFW/glfw3.h
    PATHS
      "$ENV{EMSCRIPTEN_ROOT}/cache/sysroot/include"
      "$ENV{EMSCRIPTEN_ROOT}/system/include"
      "/usr/share/emscripten/cache/sysroot/include"
      "/usr/share/emscripten/system/include"
    NO_DEFAULT_PATH
  )
  if (EMSCRIPTEN_GLFW_INCLUDE_DIR)
    target_include_directories(client_lib PUBLIC ${EMSCRIPTEN_GLFW_INCLUDE_DIR})
    target_compile_options(client_lib PUBLIC "-I${EMSCRIPTEN_GLFW_INCLUDE_DIR}")
  endif()
endif()

target_link_libraries(client_lib PUBLIC glfw)
if (NOT (BUILD_WEBASM_CLIENT_TRANSPORT AND CMAKE_SYSTEM_NAME STREQUAL "Emscripten"))
  target_link_libraries(client_lib PUBLIC glad)
endif()
target_link_libraries(client_lib PUBLIC glm::glm)
target_link_libraries(client_lib PRIVATE SimpleIni)

# TODO: check if PNG, ZLIB and BZip2 are needed for linking here
target_link_libraries(client_lib PRIVATE PNG::PNG)
target_link_libraries(client_lib PRIVATE ZLIB::ZLIB)
target_link_libraries(client_lib PRIVATE BZip2::BZip2)

target_link_libraries(client_lib PUBLIC Freetype::Freetype)
target_link_libraries(client_lib PRIVATE Spdlog)
target_link_libraries(client_lib PRIVATE imgui::imgui)
target_link_libraries(client_lib PUBLIC shared_lib)
if (NOT BUILD_WEBASM_CLIENT_TRANSPORT)
    target_link_libraries(client_lib PRIVATE GameNetworkingSockets2)
endif()
target_link_libraries(client_lib PRIVATE cxxopts::cxxopts)
soldank_target_precompile_headers(client_lib)

# TODO: uncomment this to stop the app from opening cmd on windows
# IF (WIN32)
#    add_executable(client WIN32 main.cpp)
# ELSE()
   add_executable(client main.cpp)
# ENDIF()
target_compile_definitions(client PUBLIC ${CLIENT_COMPILE_DEFINITIONS})
if (BUILD_WEBASM_CLIENT_TRANSPORT)
    target_compile_definitions(client PUBLIC SOLDANK_WEBASM_CLIENT_TRANSPORT)
endif()

if(MSVC)
  target_compile_options(client PRIVATE /W4)
  set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(client PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

target_link_libraries(client PRIVATE client_lib)
if (NOT BUILD_WEBASM_CLIENT_TRANSPORT)
    target_link_libraries(client PRIVATE GameNetworkingSockets2)
endif()
target_link_libraries(client PRIVATE Spdlog)
target_link_libraries(client PRIVATE cxxopts::cxxopts)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
if (BUILD_WEBASM_CLIENT_TRANSPORT AND CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set_target_properties(client PROPERTIES SUFFIX ".html")

  set(webasm_preload_options)
  if (BUILD_WEBASM_PRELOAD_ASSETS)
    function(soldank_webasm_preload_directory asset_directory virtual_directory)
      foreach(asset_root
          "${CMAKE_SOURCE_DIR}"
          "${soldatbase_SOURCE_DIR}/shared"
      )
        if (EXISTS "${asset_root}/${asset_directory}")
          list(APPEND webasm_preload_options
            "SHELL:--preload-file ${asset_root}/${asset_directory}@/${virtual_directory}"
          )
          set(webasm_preload_options "${webasm_preload_options}" PARENT_SCOPE)
          return()
        endif()
      endforeach()
    endfunction()

    function(soldank_webasm_preload_file asset_file virtual_file)
      foreach(asset_root
          "${CMAKE_SOURCE_DIR}"
          "${soldatbase_SOURCE_DIR}"
          "${soldatbase_SOURCE_DIR}/shared"
          "${soldatbase_SOURCE_DIR}/shared/objects"
          "${soldatbase_SOURCE_DIR}/client/configs"
      )
        if (EXISTS "${asset_root}/${asset_file}")
          list(APPEND webasm_preload_options
            "SHELL:--preload-file ${asset_root}/${asset_file}@/${virtual_file}"
          )
          set(webasm_preload_options "${webasm_preload_options}" PARENT_SCOPE)
          return()
        endif()
      endforeach()
    endfunction()

    foreach(asset_directory
        anims
        maps
        textures
        gostek-gfx
        weapons-gfx
        interface-gfx
        objects-gfx
        scenery-gfx
    )
      soldank_webasm_preload_directory("${asset_directory}" "${asset_directory}")
    endforeach()

    foreach(asset_directory
        objects
    )
      soldank_webasm_preload_directory("${asset_directory}" "${asset_directory}")
    endforeach()

    foreach(asset_file
        debug_config.ini
        play-regular.ttf
        weapons.ini
        weapons_realistic.ini
    )
      soldank_webasm_preload_file("${asset_file}" "${asset_file}")
    endforeach()

    foreach(asset_file
        gostek.po
    )
      soldank_webasm_preload_file("${asset_file}" "${asset_file}")
      if (EXISTS "${soldatbase_SOURCE_DIR}/shared/objects/${asset_file}")
        list(APPEND webasm_preload_options
          "SHELL:--preload-file ${soldatbase_SOURCE_DIR}/shared/objects/${asset_file}@/objects/${asset_file}"
        )
      endif()
    endforeach()
  endif()

  target_link_options(client PRIVATE
    "SHELL:--js-library ${CMAKE_CURRENT_SOURCE_DIR}/networking/browser/webrtc_bridge.js"
    "SHELL:-sUSE_GLFW=3"
    "SHELL:-sUSE_WEBGL2=1"
    "SHELL:-sMIN_WEBGL_VERSION=2"
    "SHELL:-sMAX_WEBGL_VERSION=2"
    "SHELL:-sGL_ASSERTIONS=1"
    # Keep wasm memory fixed so Emscripten's TextDecoder path does not see
    # resizable ArrayBuffer views in browsers.
    "SHELL:-sALLOW_MEMORY_GROWTH=0"
    "SHELL:-sINITIAL_MEMORY=536870912"
    "SHELL:-sASYNCIFY=1"
    ${webasm_preload_options}
  )
endif()
soldank_target_precompile_headers(client)
install(TARGETS client DESTINATION ${CMAKE_SOURCE_DIR}/bin)
