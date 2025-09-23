add_library(DaScript)

target_sources(DaScript
    PUBLIC
        ./DaScriptInit.cpp
)
target_sources(DaScript
    PUBLIC
        ./DaScriptInit.hpp
)
target_sources(DaScript
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        DaScript.cpp
)

target_compile_definitions(DaScript PRIVATE ${SERVER_COMPILE_DEFINITIONS})

if(MSVC)
    target_compile_options(DaScript PRIVATE /Wall)
else()
    target_compile_options(DaScript PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(DaScript PUBLIC ${dascript_SOURCE_DIR}/include)
target_link_libraries(DaScript PUBLIC libDaScript)
add_dependencies(DaScript libDaScript)

