file(GLOB_RECURSE LIB_SOURCES *.cpp)
file(GLOB_RECURSE HEADER_SOURCES *.hpp)
list(REMOVE_ITEM LIB_SOURCES main.cpp)

add_library(shared_lib ${LIB_SOURCES} ${HEADER_SOURCES})

target_compile_definitions(shared_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS} -DNOMINMAX)

if(MSVC)
  target_compile_options(shared_lib PRIVATE /Wall)
else()
  target_compile_options(shared_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(shared_lib PUBLIC ${SIMPLEINI_INCLUDE_DIRS})
target_include_directories(shared_lib PUBLIC ${CMAKE_SOURCE_DIR}/source/shared)

target_link_libraries(shared_lib PUBLIC glm::glm)
target_link_libraries(shared_lib PRIVATE spdlog::spdlog)
