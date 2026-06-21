add_library(Httplib)

target_sources(Httplib
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        Httplib.cpp
)

if(MSVC)
    target_compile_options(Httplib PRIVATE /Wall)
else()
    target_compile_options(Httplib PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(Httplib PUBLIC httplib::httplib)

if(UNIX AND NOT APPLE)
    target_link_libraries(Httplib PUBLIC anl)
endif()
