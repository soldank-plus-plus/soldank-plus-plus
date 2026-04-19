set(client_lib_modules
    ./stb.cpp

    ./application/Application.cpp
    ./application/cli/CommandLineParameters.cpp
    ./application/window/Window.cpp
    ./application/input/Keyboard.cpp
    ./application/input/Mouse.cpp
    ./application/input/PlayerInput.cpp

    ./map_editor/MapEditor.cpp
    ./map_editor/actions/MapEditorAction.cpp
    ./map_editor/actions/AddObjectsMapEditorAction.cpp
    ./map_editor/actions/AddPolygonMapEditorAction.cpp
    ./map_editor/actions/AddSceneryMapEditorAction.cpp
    ./map_editor/actions/AddSpawnPointMapEditorAction.cpp
    ./map_editor/actions/ColorObjectsMapEditorAction.cpp
    ./map_editor/actions/MoveSelectionMapEditorAction.cpp
    ./map_editor/actions/RemoveSelectionMapEditorAction.cpp
    ./map_editor/actions/RotateSelectionMapEditorAction.cpp
    ./map_editor/actions/ScaleSelectionMapEditorAction.cpp
    ./map_editor/actions/TransformPolygonsMapEditorAction.cpp
    ./map_editor/actions/TransformSceneriesMapEditorAction.cpp
    ./map_editor/actions/TransformSpawnPointsMapEditorAction.cpp
    ./map_editor/tools/ColorPickerTool.cpp
    ./map_editor/tools/ColorTool.cpp
    ./map_editor/tools/PolygonTool.cpp
    ./map_editor/tools/SceneryTool.cpp
    ./map_editor/tools/SelectionTool.cpp
    ./map_editor/tools/SpawnpointTool.cpp
    ./map_editor/tools/TextureTool.cpp
    ./map_editor/tools/Tool.cpp
    ./map_editor/tools/TransformTool.cpp
    ./map_editor/tools/VertexColorTool.cpp
    ./map_editor/tools/VertexSelectionTool.cpp
    ./map_editor/tools/WaypointTool.cpp

    ./networking/Connection.cpp
    ./networking/IConnection.cpp
    ./networking/NetworkingClient.cpp
    ./networking/INetworkingClient.cpp
    ./networking/event_handlers/AssignPlayerIdNetworkEventHandler.cpp
    ./networking/event_handlers/HitSoldierNetworkEventHandler.cpp
    ./networking/event_handlers/KillSoldierNetworkEventHandler.cpp
    ./networking/event_handlers/PingCheckNetworkEventHandler.cpp
    ./networking/event_handlers/PlayerLeaveNetworkEventHandler.cpp
    ./networking/event_handlers/ProjectileSpawnNetworkEventHandler.cpp
    ./networking/event_handlers/SoldierInfoNetworkEventHandler.cpp
    ./networking/event_handlers/SoldierStateNetworkEventHandler.cpp
    ./networking/event_handlers/SpawnSoldierNetworkEventHandler.cpp
    ./networking/interface/NetworkingInterface.cpp

    ./rendering/Scene.cpp
    ./rendering/ClientState.cpp
    ./rendering/components/Camera.cpp
    ./rendering/data/Texture.cpp
    ./rendering/data/sprites/SoldierPartData.cpp
    ./rendering/data/sprites/SpritesManager.cpp
    ./rendering/renderer/BackgroundRenderer.cpp
    ./rendering/renderer/BulletRenderer.cpp
    ./rendering/renderer/CircleRenderer.cpp
    ./rendering/renderer/ItemRenderer.cpp
    ./rendering/renderer/LineRenderer.cpp
    ./rendering/renderer/PolygonOutlinesRenderer.cpp
    ./rendering/renderer/PolygonsRenderer.cpp
    ./rendering/renderer/RectangleRenderer.cpp
    ./rendering/renderer/Renderer.cpp
    ./rendering/renderer/SceneriesRenderer.cpp
    ./rendering/renderer/SceneryOutlinesRenderer.cpp
    ./rendering/renderer/SoldierRenderer.cpp
    ./rendering/renderer/interface/CursorRenderer.cpp
    ./rendering/renderer/interface/ImGuiThemes.cpp
    ./rendering/renderer/interface/debug/DebugUI.cpp
    ./rendering/renderer/interface/map_editor/GridRenderer.cpp
    ./rendering/renderer/interface/map_editor/MapEditorScene.cpp
    ./rendering/renderer/interface/map_editor/MapEditorToolDetailsWindow.cpp
    ./rendering/renderer/interface/map_editor/MapEditorUI.cpp
    ./rendering/renderer/interface/map_editor/PolygonVertexOutlinesRenderer.cpp
    ./rendering/renderer/interface/map_editor/SingleImageRenderer.cpp
    ./rendering/renderer/interface/map_editor/SpawnPointRenderer.cpp
    ./rendering/renderer/TextRenderer.cpp
    ./rendering/shaders/Shader.cpp
)

foreach(file_path IN LISTS client_lib_modules)
    get_filename_component(absolute_path ${file_path} ABSOLUTE)
    message(${file_path})
    list(REMOVE_ITEM LIB_SOURCES ${absolute_path})
endforeach()

add_library(client_lib)

target_sources(client_lib
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        ${client_lib_modules}
)

target_compile_definitions(client_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(client_lib PRIVATE /Wall)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(client_lib PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

target_include_directories(client_lib PUBLIC ${SIMPLEINI_INCLUDE_DIRS})
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
target_include_directories(client_lib PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(client_lib PUBLIC glfw)
target_link_libraries(client_lib PUBLIC glad)
target_link_libraries(client_lib PUBLIC glm::glm)

# TODO: check if PNG, ZLIB and BZip2 are needed for linking here
target_link_libraries(client_lib PRIVATE PNG::PNG)
target_link_libraries(client_lib PRIVATE ZLIB::ZLIB)
target_link_libraries(client_lib PRIVATE BZip2::BZip2)

target_link_libraries(client_lib PUBLIC Freetype::Freetype)
target_link_libraries(client_lib PRIVATE Spdlog)
target_link_libraries(client_lib PRIVATE imgui::imgui)
target_link_libraries(client_lib PUBLIC shared_lib)
target_link_libraries(client_lib PRIVATE GameNetworkingSockets2)
target_link_libraries(client_lib PRIVATE cxxopts::cxxopts)

# TODO: uncomment this to stop the app from opening cmd on windows
# IF (WIN32)
#    add_executable(client WIN32 main.cpp)
# ELSE()
   add_executable(client main.cpp)
# ENDIF()
target_compile_definitions(client PUBLIC ${CLIENT_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(client PRIVATE /Wall)
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
target_link_libraries(client PRIVATE GameNetworkingSockets2)
target_link_libraries(client PRIVATE Spdlog)
target_link_libraries(client PRIVATE cxxopts::cxxopts)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
install(TARGETS client DESTINATION ${CMAKE_SOURCE_DIR}/bin)
