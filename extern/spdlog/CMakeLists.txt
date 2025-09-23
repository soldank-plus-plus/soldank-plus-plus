add_library(Spdlog)

target_sources(Spdlog
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        Spdlog.cpp
)

if(MSVC)
    target_compile_options(Spdlog PRIVATE /Wall)
else()
    target_compile_options(Spdlog PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(Spdlog PRIVATE spdlog::spdlog)


