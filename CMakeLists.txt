cmake_minimum_required (VERSION 3.28)

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake )
	message(STATUS "Found ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake; using it!")
	set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
		CACHE STRING "Vcpkg toolchain file")
endif()

project (Soldank++)

option(BUILD_SERVER_ENABLED "Build server" ON)
option(BUILD_CLIENT_ENABLED "Build client" ON)
option(BUILD_TESTS_ENABLED "Build all available tests" ON)
option(BUILD_COMPILE_WITH_COVERAGE "Build with gcov-compatible coverage instrumentation" OFF)
option(BUILD_USE_CCACHE "Use ccache as compiler launcher when available" ON)

if (BUILD_USE_CCACHE AND NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}; using it as compiler launcher")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache requested but not found")
    endif()
endif()

if (WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
endif()

if (MSVC)
    add_compile_options(/utf-8)
endif()

if (BUILD_COMPILE_WITH_COVERAGE)
    message("COMPILE WITH COVERAGE ON")
else()
    message("COMPILE WITH COVERAGE OFF")
endif()

if (NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()

find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

if (BUILD_CLIENT_ENABLED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(Stb REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(imgui CONFIG REQUIRED)
endif()

if (BUILD_TESTS_ENABLED)
    find_package(GTest CONFIG REQUIRED)
endif()

if (BUILD_SERVER_ENABLED)
    find_package(httplib CONFIG REQUIRED)
endif()

include(cmake/CPM.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

CPMAddPackage(
    NAME soldatbase
    URL https://github.com/nedik/soldat-base/archive/55c6867a943a6ffc62a57b46eb7742c2e39766aa.tar.gz
)

if (BUILD_SERVER_ENABLED)
    # Daslang is still in development and is not released in vcpkg so we have to use CPM
    # Using version v0.6.3-RC2 commit
    set(DAS_AOT_EXAMPLES_DISABLED ON CACHE BOOL "Disable daScript AOT examples" FORCE)
    set(DAS_GLFW_DISABLED ON CACHE BOOL "Disable daScript GLFW module" FORCE)

    CPMAddPackage(
        NAME dascript
        OPTIONS 
            "DAS_TOOLS_DISABLED ON"
            "DAS_TESTS_DISABLED ON"
            "DAS_PROFILE_DISABLED ON"
            "DAS_TUTORIAL_DISABLED ON"
            "DAS_AOT_EXAMPLES_DISABLED ON"
            "DAS_GLFW_DISABLED ON"
        URL https://github.com/GaijinEntertainment/daScript/archive/8b59954b1acfa88e3ddec96146330a36f12a3b7a.tar.gz
        EXCLUDE_FROM_ALL ON
    )

    if(dascript_ADDED)
        target_link_libraries(libDaScript Threads::Threads)
    endif()
endif()

add_subdirectory(extern)
add_subdirectory(source)
if (BUILD_TESTS_ENABLED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
