add_subdirectory(glad-2.1.0)

if (BUILD_SERVER_ENABLED)
    add_subdirectory(DaScript)
endif()
