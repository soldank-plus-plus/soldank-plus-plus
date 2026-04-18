add_library(GameNetworkingSockets2)

target_sources(GameNetworkingSockets2
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES
        GameNetworkingSockets.cpp
)

if(MSVC)
    target_compile_options(GameNetworkingSockets2 PRIVATE /Wall)
else()
    target_compile_options(GameNetworkingSockets2 PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(GameNetworkingSockets2 PRIVATE GameNetworkingSockets::GameNetworkingSockets_s)

