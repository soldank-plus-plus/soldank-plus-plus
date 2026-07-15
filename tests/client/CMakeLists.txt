add_executable(MapEditorActionsTest map_editor/MapEditorActionsTest.cpp)
target_link_libraries(MapEditorActionsTest PRIVATE client_lib shared_lib)
AddTestOptionsAndLibraries(MapEditorActionsTest)
add_custom_command(TARGET MapEditorActionsTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "$<TARGET_FILE_DIR:MapEditorActionsTest>/weapons.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "$<TARGET_FILE_DIR:MapEditorActionsTest>/weapons_realistic.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/anims"
    "$<TARGET_FILE_DIR:MapEditorActionsTest>/anims"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:MapEditorActionsTest>/objects"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
    "$<TARGET_FILE_DIR:MapEditorActionsTest>/objects/gostek.po"
)

add_executable(MapEditorToolsTest map_editor/MapEditorToolsTest.cpp)
target_link_libraries(MapEditorToolsTest PRIVATE client_lib shared_lib)
AddTestOptionsAndLibraries(MapEditorToolsTest)
add_custom_command(TARGET MapEditorToolsTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "$<TARGET_FILE_DIR:MapEditorToolsTest>/weapons.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "$<TARGET_FILE_DIR:MapEditorToolsTest>/weapons_realistic.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/anims"
    "$<TARGET_FILE_DIR:MapEditorToolsTest>/anims"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:MapEditorToolsTest>/objects"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
    "$<TARGET_FILE_DIR:MapEditorToolsTest>/objects/gostek.po"
)

add_test(NAME MapEditorActionsTest COMMAND MapEditorActionsTest)
add_test(NAME MapEditorToolsTest COMMAND MapEditorToolsTest)
add_executable(MapEditorControllersTest map_editor/MapEditorControllersTest.cpp)
target_link_libraries(MapEditorControllersTest PRIVATE client_lib shared_lib)
AddTestOptionsAndLibraries(MapEditorControllersTest)
add_custom_command(TARGET MapEditorControllersTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "$<TARGET_FILE_DIR:MapEditorControllersTest>/weapons.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "$<TARGET_FILE_DIR:MapEditorControllersTest>/weapons_realistic.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/anims"
    "$<TARGET_FILE_DIR:MapEditorControllersTest>/anims"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:MapEditorControllersTest>/objects"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
    "$<TARGET_FILE_DIR:MapEditorControllersTest>/objects/gostek.po"
)

add_test(NAME MapEditorControllersTest COMMAND MapEditorControllersTest)
set_tests_properties(MapEditorActionsTest PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:MapEditorActionsTest>")
set_tests_properties(MapEditorToolsTest PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:MapEditorToolsTest>")
set_tests_properties(MapEditorControllersTest PROPERTIES
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:MapEditorControllersTest>")
