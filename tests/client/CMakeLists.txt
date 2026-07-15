set(MAP_EDITOR_TEST_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/map_editor_test_data")
set(MAP_EDITOR_TEST_DATA_STAMP "${MAP_EDITOR_TEST_DATA_DIR}/assets.stamp")

add_custom_command(
  OUTPUT "${MAP_EDITOR_TEST_DATA_STAMP}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MAP_EDITOR_TEST_DATA_DIR}/objects"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "${MAP_EDITOR_TEST_DATA_DIR}/weapons.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "${MAP_EDITOR_TEST_DATA_DIR}/weapons_realistic.ini"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${soldatbase_SOURCE_DIR}/shared/anims"
    "${MAP_EDITOR_TEST_DATA_DIR}/anims"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
    "${MAP_EDITOR_TEST_DATA_DIR}/objects/gostek.po"
  COMMAND ${CMAKE_COMMAND} -E touch "${MAP_EDITOR_TEST_DATA_STAMP}"
  DEPENDS
    "${soldatbase_SOURCE_DIR}/shared/weapons.ini"
    "${soldatbase_SOURCE_DIR}/shared/weapons_realistic.ini"
    "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po"
  VERBATIM
)
add_custom_target(MapEditorTestData DEPENDS "${MAP_EDITOR_TEST_DATA_STAMP}")

function(AddMapEditorTest TestName SourceFile)
  add_executable(${TestName} ${SourceFile})
  target_link_libraries(${TestName} PRIVATE client_lib shared_lib)
  AddTestOptionsAndLibraries(${TestName})
  add_dependencies(${TestName} MapEditorTestData)
  add_test(NAME ${TestName} COMMAND ${TestName})
  set_tests_properties(${TestName} PROPERTIES
    WORKING_DIRECTORY "${MAP_EDITOR_TEST_DATA_DIR}"
  )
endfunction()

AddMapEditorTest(MapEditorActionsTest map_editor/MapEditorActionsTest.cpp)
AddMapEditorTest(MapEditorToolsTest map_editor/MapEditorToolsTest.cpp)
AddMapEditorTest(MapEditorControllersTest map_editor/MapEditorControllersTest.cpp)
