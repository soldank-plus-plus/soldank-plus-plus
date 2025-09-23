add_subdirectory(glad-2.1.0)
add_subdirectory(simple_ini)
add_subdirectory(spdlog)

if (BUILD_SERVER_ENABLED)
    add_subdirectory(DaScript)
endif()
