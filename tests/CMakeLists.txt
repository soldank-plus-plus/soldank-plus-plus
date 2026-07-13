MACRO(AddTestOptionsAndLibraries TestName)
  if (BUILD_COMPILE_WITH_COVERAGE)
    target_compile_options(${TestName} PRIVATE -coverage -fprofile-arcs -ftest-coverage)
    target_link_options(${TestName} PRIVATE -coverage -fprofile-arcs)
  endif()
  target_link_libraries(${TestName} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
  if (BUILD_USE_PCH AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 21))
    target_precompile_headers(${TestName} PRIVATE
      <gmock/gmock.h>
      <gtest/gtest.h>
      <memory>
      <optional>
      <string>
      <vector>
    )
  endif()
ENDMACRO()

add_subdirectory(testing_frameworks/shared_lib_testing)
add_subdirectory(shared)
if (BUILD_CLIENT_ENABLED)
  add_subdirectory(client)
endif()
