set(MAP_EDITOR_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/map_editor_test_data")
set(MAP_EDITOR_TEST_DATA_VERSION 3)
set(MAP_EDITOR_TEST_DATA_STAMP
  "${MAP_EDITOR_TEST_DATA_DIR}/assets-v${MAP_EDITOR_TEST_DATA_VERSION}.stamp")

add_custom_command(
  OUTPUT "${MAP_EDITOR_TEST_DATA_STAMP}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MAP_EDITOR_TEST_DATA_DIR}/objects"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MAP_EDITOR_TEST_DATA_DIR}/textures"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MAP_EDITOR_TEST_DATA_DIR}/scenery-gfx"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "${MAP_EDITOR_TEST_DATA_DIR}/weapons.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "${MAP_EDITOR_TEST_DATA_DIR}/weapons_realistic.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/anims"
    "${MAP_EDITOR_TEST_DATA_DIR}/anims"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/objects"
    "${MAP_EDITOR_TEST_DATA_DIR}/objects"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/play-regular.ttf"
    "${MAP_EDITOR_TEST_DATA_DIR}/play-regular.ttf"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/textures/malina.png"
    "${MAP_EDITOR_TEST_DATA_DIR}/textures/malina.png"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/scenery-gfx/rir-hotel2.png"
    "${MAP_EDITOR_TEST_DATA_DIR}/scenery-gfx/rir-hotel2.png"
  COMMAND ${CMAKE_COMMAND} -E touch "${MAP_EDITOR_TEST_DATA_STAMP}"
  DEPENDS
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
    "${soldatbase_SOURCE_DIR}/play-regular.ttf"
    "${soldatbase_SOURCE_DIR}/shared/textures/malina.png"
    "${soldatbase_SOURCE_DIR}/shared/scenery-gfx/rir-hotel2.png"
  VERBATIM
)
add_custom_target(MapEditorTestData DEPENDS "${MAP_EDITOR_TEST_DATA_STAMP}")

function(AddClientTestWithMapEditorData TestName SourceFile)
  add_executable(${TestName} ${SourceFile})
  target_link_libraries(${TestName} PRIVATE client_lib shared_lib)
  AddTestOptionsAndLibraries(${TestName})
  add_dependencies(${TestName} MapEditorTestData)
  add_test(NAME ${TestName} COMMAND ${TestName})
  set_tests_properties(${TestName} PROPERTIES
    WORKING_DIRECTORY "${MAP_EDITOR_TEST_DATA_DIR}"
  )
endfunction()

AddClientTestWithMapEditorData(MapEditorActionsTest map_editor/MapEditorActionsTest.cpp)
AddClientTestWithMapEditorData(MapEditorToolsTest map_editor/MapEditorToolsTest.cpp)
AddClientTestWithMapEditorData(MapEditorControllersTest map_editor/MapEditorControllersTest.cpp)
AddClientTestWithMapEditorData(WeaponAudioTest audio/WeaponAudioTest.cpp)
target_link_libraries(WeaponAudioTest PRIVATE shared_lib_testing_framework)

if (BUILD_IMGUI_TEST_ENGINE_ENABLED)
  AddClientTestWithMapEditorData(
    ImGuiInterfaceTest
    rendering/renderer/interface/ImGuiInterfaceTest.cpp
  )
  target_link_libraries(ImGuiInterfaceTest PRIVATE imgui::imgui)
endif()

add_executable(ShortcutTest application/input/ShortcutTest.cpp)
target_link_libraries(ShortcutTest PRIVATE client_lib)
AddTestOptionsAndLibraries(ShortcutTest)
add_test(NAME ShortcutTest COMMAND ShortcutTest)

add_executable(InputApplicationTimelineTest networking/InputApplicationTimelineTest.cpp)
target_link_libraries(InputApplicationTimelineTest PRIVATE client_lib)
AddTestOptionsAndLibraries(InputApplicationTimelineTest)
add_test(NAME InputApplicationTimelineTest COMMAND InputApplicationTimelineTest)

add_executable(ReconciliationTimelineTest networking/ReconciliationTimelineTest.cpp)
target_link_libraries(ReconciliationTimelineTest PRIVATE client_lib)
AddTestOptionsAndLibraries(ReconciliationTimelineTest)
add_test(NAME ReconciliationTimelineTest COMMAND ReconciliationTimelineTest)

if (BUILD_SERVER_ENABLED)
  add_executable(NetworkedInputSimulationTest networking/NetworkedInputSimulationTest.cpp)
  target_link_libraries(NetworkedInputSimulationTest PRIVATE client_lib server_lib)
  AddTestOptionsAndLibraries(NetworkedInputSimulationTest)
  add_test(NAME NetworkedInputSimulationTest COMMAND NetworkedInputSimulationTest)

  add_executable(FullClientServerReconciliationTest
    networking/FullClientServerReconciliationTest.cpp)
  target_link_libraries(FullClientServerReconciliationTest
    PRIVATE client_lib server_lib shared_lib_testing_framework)
  AddTestOptionsAndLibraries(FullClientServerReconciliationTest)
  add_dependencies(FullClientServerReconciliationTest MapEditorTestData)
  add_test(NAME FullClientServerReconciliationTest COMMAND FullClientServerReconciliationTest)
  set_tests_properties(FullClientServerReconciliationTest PROPERTIES
    WORKING_DIRECTORY "${MAP_EDITOR_TEST_DATA_DIR}"
  )
endif()

AddClientTestWithMapEditorData(PolygonReconciliationTest networking/PolygonReconciliationTest.cpp)
target_link_libraries(PolygonReconciliationTest PRIVATE shared_lib_testing_framework)

AddClientTestWithMapEditorData(VisualEffectsTest rendering/VisualEffectsTest.cpp)
target_link_libraries(VisualEffectsTest PRIVATE shared_lib_testing_framework)

# Optional software OpenGL coverage: no window server or graphics hardware is required.
if (UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  find_package(OpenGL QUIET COMPONENTS EGL)
  if (TARGET OpenGL::EGL)
    add_executable(VisualEffectsRenderingTest rendering/VisualEffectsRenderingTest.cpp)
    target_link_libraries(VisualEffectsRenderingTest PRIVATE client_lib OpenGL::EGL)
    AddTestOptionsAndLibraries(VisualEffectsRenderingTest)
    add_test(NAME VisualEffectsRenderingTest COMMAND VisualEffectsRenderingTest)
    set_tests_properties(VisualEffectsRenderingTest PROPERTIES
      WORKING_DIRECTORY "${soldatbase_SOURCE_DIR}/shared"
      ENVIRONMENT "LIBGL_ALWAYS_SOFTWARE=1;EGL_PLATFORM=surfaceless;MESA_SHADER_CACHE_DISABLE=true"
    )
  endif()
endif()

AddClientTestWithMapEditorData(GameModeProtocolTest networking/GameModeProtocolTest.cpp)

AddClientTestWithMapEditorData(ChatTest chat/ChatTest.cpp)
target_link_libraries(ChatTest PRIVATE imgui::imgui)
add_custom_command(TARGET ChatTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets/fonts" "${MAP_EDITOR_TEST_DATA_DIR}/fonts")

if (BUILD_SERVER_ENABLED)
    add_executable(ChatIntegrationTest chat/ChatIntegrationTest.cpp)
    AddTestOptionsAndLibraries(ChatIntegrationTest)
    target_link_libraries(ChatIntegrationTest PRIVATE client_lib server_lib shared_lib)
    add_test(NAME ChatIntegrationTest COMMAND ChatIntegrationTest)
endif()
