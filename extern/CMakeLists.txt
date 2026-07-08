add_subdirectory(simple_ini)
add_subdirectory(spdlog)
add_subdirectory(glm)

if (BUILD_CLIENT_ENABLED)
    add_subdirectory(glad-2.1.0)
endif()

if ((BUILD_CLIENT_ENABLED AND NOT BUILD_WEBASM_CLIENT_TRANSPORT) OR BUILD_SERVER_ENABLED)
    add_subdirectory(game_networking_sockets)
endif()

if (BUILD_SERVER_ENABLED)
    add_subdirectory(DaScript)
    add_subdirectory(httplib)
endif()
