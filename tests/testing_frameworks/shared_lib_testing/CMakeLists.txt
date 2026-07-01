file(GLOB_RECURSE LIB_SOURCES *.cpp)
file(GLOB_RECURSE HEADER_SOURCES *.hpp)

add_library(shared_lib_testing_framework ${HEADER_SOURCES})
target_sources(shared_lib_testing_framework
    PUBLIC
        FILE_SET all_shared_lib_testing_framework_modules TYPE CXX_MODULES FILES
        ${LIB_SOURCES}
)

target_compile_definitions(shared_lib_testing_framework PUBLIC ${CLIENT_COMPILE_DEFINITIONS} -DNOMINMAX)

if(MSVC)
  target_compile_options(shared_lib_testing_framework PRIVATE /W4)
else()
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(shared_lib_testing_framework PRIVATE -Wall -Wextra -Wpedantic -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(shared_lib_testing_framework PRIVATE -coverage -fprofile-arcs)
  else()
    target_compile_options(shared_lib_testing_framework PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

target_include_directories(shared_lib_testing_framework PUBLIC ${CMAKE_SOURCE_DIR}/tests/testing_frameworks)

target_link_libraries(shared_lib_testing_framework PRIVATE shared_lib)

soldank_target_precompile_headers(shared_lib_testing_framework)
