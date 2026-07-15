add_library(TomlPlusPlus)

target_sources(TomlPlusPlus
    PUBLIC
        FILE_SET all_tomlplusplus_modules TYPE CXX_MODULES FILES
        TomlPlusPlus.cpp
)

if(MSVC)
    target_compile_options(TomlPlusPlus PRIVATE /W4)
else()
    target_compile_options(TomlPlusPlus PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(TomlPlusPlus PRIVATE tomlplusplus::tomlplusplus)
