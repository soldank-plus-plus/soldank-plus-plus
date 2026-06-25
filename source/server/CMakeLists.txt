set(server_lib_modules
    application/Application.cpp
    application/ServerState.cpp

    scripting/IScriptingEngine.cpp
    scripting/dascript/DaScriptScriptingEngine.cpp

    networking/IGameServer.cpp
    networking/GameServer.cpp
    networking/CoreEventsConnectionNotifier.cpp
    networking/LobbyClient.cpp
    networking/event_handlers/KillCommandNetworkEventHandler.cpp
    networking/event_handlers/PingCheckNetworkEventHandler.cpp
    networking/event_handlers/SoldierInputNetworkEventHandler.cpp
    networking/interface/NetworkingInterface.cpp
    networking/poll_groups/IPollGroup.cpp
    networking/poll_groups/PollGroupBase.cpp
    networking/poll_groups/EntryPollGroup.cpp
    networking/poll_groups/PlayerPollGroup.cpp
    networking/types/Connection.cpp
)

add_library(server_lib)
target_sources(server_lib
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        ${server_lib_modules}
)

target_compile_definitions(server_lib PRIVATE ${SERVER_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(server_lib PRIVATE /W4)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(server_lib PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(server_lib PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(server_lib PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

target_include_directories(server_lib PRIVATE ${CMAKE_SOURCE_DIR}/source/server)

target_link_libraries(server_lib PRIVATE shared_lib)
target_link_libraries(server_lib PRIVATE GameNetworkingSockets2)
target_link_libraries(server_lib PRIVATE Spdlog)
target_link_libraries(server_lib PRIVATE DaScript)
target_link_libraries(server_lib PRIVATE SimpleIni)
target_link_libraries(server_lib PRIVATE Httplib)
target_link_libraries(server_lib PRIVATE Threads::Threads)

add_executable(server main.cpp)

target_compile_definitions(server PRIVATE ${SERVER_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(server PRIVATE /W4)
  set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(server PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

target_link_libraries(server PRIVATE shared_lib)
target_link_libraries(server PRIVATE server_lib)
target_link_libraries(server PRIVATE GameNetworkingSockets2)
target_link_libraries(server PRIVATE Spdlog)
target_link_libraries(server PRIVATE DaScript)
target_link_libraries(server PRIVATE Httplib)
target_link_libraries(server PRIVATE Threads::Threads)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/source/server)

install(TARGETS server DESTINATION ${CMAKE_SOURCE_DIR}/bin)
