add_library(movement_scenarios_lib)
target_sources(movement_scenarios_lib PUBLIC
    FILE_SET movement_scenario_modules TYPE CXX_MODULES FILES
    ScenarioId.cpp ScenarioDefinition.cpp ScenarioRunner.cpp ScenarioExport.cpp ScenarioCatalog.cpp)
target_link_libraries(movement_scenarios_lib PUBLIC shared_lib)
# Reuse the JSON implementation shipped with the pinned toml++ dependency.
target_include_directories(movement_scenarios_lib PRIVATE "${tomlplusplus_SOURCE_DIR}/vendor")
target_compile_definitions(movement_scenarios_lib PRIVATE
    MOVEMENT_SCENARIO_BUILD_ID="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${CMAKE_BUILD_TYPE}")

function(stage_movement_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${soldatbase_SOURCE_DIR}/shared" "$<TARGET_FILE_DIR:${target_name}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${soldatbase_SOURCE_DIR}/shared/objects/gostek.po" "$<TARGET_FILE_DIR:${target_name}>/gostek.po")
endfunction()
