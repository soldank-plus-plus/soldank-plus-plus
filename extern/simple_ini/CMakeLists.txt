add_library(SimpleIni)

target_sources(SimpleIni
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES 
        SimpleIni.cpp
)

if(MSVC)
    target_compile_options(SimpleIni PRIVATE /Wall)
else()
    target_compile_options(SimpleIni PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(SimpleIni PRIVATE ${SIMPLEINI_INCLUDE_DIRS})

