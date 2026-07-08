add_library(Glm)

target_sources(Glm
    PUBLIC
        FILE_SET all_my_modules TYPE CXX_MODULES FILES
        Glm.cpp
)

if(MSVC)
    target_compile_options(Glm PRIVATE /W4)
else()
    target_compile_options(Glm PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(Glm PUBLIC glm::glm)
