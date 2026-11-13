add_library(Miniaudio)

target_sources(Miniaudio
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES
        Miniaudio.cpp
    PRIVATE
        MiniaudioImpl.cpp
)

if(MSVC)
    target_compile_options(Miniaudio PRIVATE /W4)
else()
    target_compile_options(Miniaudio PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)
find_package(Threads REQUIRED)
target_include_directories(Miniaudio SYSTEM PRIVATE ${MINIAUDIO_INCLUDE_DIR})
target_link_libraries(Miniaudio PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(Miniaudio PRIVATE ole32)
endif()
