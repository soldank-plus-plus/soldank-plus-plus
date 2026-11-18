add_executable(movement_scenario_player main.cpp)
target_sources(movement_scenario_player PUBLIC FILE_SET player_modules TYPE CXX_MODULES FILES ScenarioPlayerApplication.cpp)
target_link_libraries(movement_scenario_player PRIVATE movement_scenarios_lib client_lib imgui::imgui)
stage_movement_assets(movement_scenario_player)
add_custom_command(TARGET movement_scenario_player POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${soldatbase_SOURCE_DIR}/play-regular.ttf" "$<TARGET_FILE_DIR:movement_scenario_player>/play-regular.ttf"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/fonts" "$<TARGET_FILE_DIR:movement_scenario_player>/fonts")

if (MSVC)
    set_target_properties(movement_scenario_player PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
else()
    set_target_properties(movement_scenario_player PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
endif()
