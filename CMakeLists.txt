cmake_minimum_required (VERSION 3.28)

cmake_policy(SET CMP0141 NEW)

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
option(BUILD_USE_COMPILER_CACHE "Use compiler cache when available" ON)
option(BUILD_USE_MOLD_LINKER "Use mold as the linker when supported" OFF)
option(BUILD_USE_LLD_LINKER "Use LLVM lld as the linker when supported" OFF)
option(BUILD_USE_PCH "Use precompiled headers for project targets" OFF)
option(BUILD_ENABLE_TIME_REPORT "Print compiler time reports for each compiled source" OFF)
option(BUILD_USE_ADDRESS_SANITIZER "Build with AddressSanitizer instrumentation" OFF)
option(BUILD_USE_UNDEFINED_BEHAVIOR_SANITIZER "Build with UndefinedBehaviorSanitizer instrumentation" OFF)

if (BUILD_USE_COMPILER_CACHE)
    if (MSVC)
        find_program(SCCACHE_PROGRAM sccache)
        if (SCCACHE_PROGRAM)
            message(STATUS "Found sccache: ${SCCACHE_PROGRAM}; using it as MSVC compiler launcher")
            set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
            set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        else()
            message(STATUS "compiler cache requested but sccache was not found")
        endif()
    else()
        find_program(CCACHE_PROGRAM ccache)
        if (CCACHE_PROGRAM)
            message(STATUS "Found ccache: ${CCACHE_PROGRAM}; using it as compiler launcher")
            set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        else()
            message(STATUS "compiler cache requested but ccache was not found")
        endif()
    endif()
endif()

if (WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
endif()

if (MSVC)
    add_compile_options(/utf-8)
endif()

if (BUILD_USE_MOLD_LINKER)
    if (MSVC)
        message(FATAL_ERROR "BUILD_USE_MOLD_LINKER is not supported for MSVC builds")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        find_program(MOLD_LINKER mold)
        if (NOT MOLD_LINKER)
            message(FATAL_ERROR "BUILD_USE_MOLD_LINKER is ON but mold was not found")
        endif()

        message(STATUS "Found mold: ${MOLD_LINKER}; using it as the linker")
        add_link_options(-fuse-ld=mold)
    else()
        message(FATAL_ERROR "BUILD_USE_MOLD_LINKER is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (BUILD_USE_LLD_LINKER)
    if (CMAKE_VERSION VERSION_LESS 3.29)
        message(FATAL_ERROR "BUILD_USE_LLD_LINKER requires CMake 3.29 or newer")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC|Clang|GNU")
        if (MSVC)
            find_program(LLD_LINKER lld-link)
        else()
            find_program(LLD_LINKER ld.lld)
        endif()

        if (NOT LLD_LINKER)
            message(FATAL_ERROR "BUILD_USE_LLD_LINKER is ON but lld was not found")
        endif()

        message(STATUS "Found lld: ${LLD_LINKER}; using it as the linker")
        set(CMAKE_LINKER_TYPE LLD)
    else()
        message(FATAL_ERROR "BUILD_USE_LLD_LINKER is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (BUILD_ENABLE_TIME_REPORT)
    if (MSVC)
        message(FATAL_ERROR "BUILD_ENABLE_TIME_REPORT is not supported for MSVC builds")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "Compiler time reports enabled")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ftime-report>)
    else()
        message(FATAL_ERROR "BUILD_ENABLE_TIME_REPORT is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (BUILD_USE_ADDRESS_SANITIZER)
    if (MSVC)
        message(FATAL_ERROR "BUILD_USE_ADDRESS_SANITIZER is not supported for MSVC builds")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "ADDRESS SANITIZER ON")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    else()
        message(FATAL_ERROR "BUILD_USE_ADDRESS_SANITIZER is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (BUILD_USE_UNDEFINED_BEHAVIOR_SANITIZER)
    if (MSVC)
        message(FATAL_ERROR "BUILD_USE_UNDEFINED_BEHAVIOR_SANITIZER is not supported for MSVC builds")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "UNDEFINED BEHAVIOR SANITIZER ON")
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
    else()
        message(FATAL_ERROR "BUILD_USE_UNDEFINED_BEHAVIOR_SANITIZER is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if (BUILD_COMPILE_WITH_COVERAGE)
    message("COMPILE WITH COVERAGE ON")
else()
    message("COMPILE WITH COVERAGE OFF")
endif()

if (NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()

function(soldank_target_precompile_headers target_name)
    if (BUILD_USE_PCH)
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 21)
            message(STATUS "Skipping PCH for ${target_name}: Clang ${CMAKE_CXX_COMPILER_VERSION} crashes when this project combines PCH with C++ module PCMs")
            return()
        endif()

        target_precompile_headers(${target_name} PRIVATE
            <algorithm>
            <array>
            <cmath>
            <cstdint>
            <memory>
            <optional>
            <string>
            <string_view>
            <utility>
            <variant>
            <vector>
            <glm/glm.hpp>
        )
    endif()
endfunction()

find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)

if (BUILD_CLIENT_ENABLED OR BUILD_SERVER_ENABLED)
    find_package(GameNetworkingSockets CONFIG REQUIRED)
    find_package(Protobuf REQUIRED)
endif()

if (BUILD_CLIENT_ENABLED)
    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(PNG REQUIRED)
    find_package(cxxopts CONFIG REQUIRED)
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

if (BUILD_TESTS_ENABLED)
    CPMAddPackage(
        NAME soldatbase
        URL https://github.com/nedik/soldat-base/archive/55c6867a943a6ffc62a57b46eb7742c2e39766aa.tar.gz
    )
endif()

if (BUILD_SERVER_ENABLED)
    # Daslang is still in development and is not released in vcpkg so we have to use CPM
    # Using version v0.6.3-RC2 commit
    set(DAS_AOT_EXAMPLES_DISABLED ON CACHE BOOL "Disable daScript AOT examples" FORCE)
    set(DAS_AUDIO_DISABLED ON CACHE BOOL "Disable daScript audio module" FORCE)
    set(DAS_GLFW_DISABLED ON CACHE BOOL "Disable daScript GLFW module" FORCE)
    set(DAS_HV_DISABLED ON CACHE BOOL "Disable daScript HV module" FORCE)
    set(DAS_LIVE_HOST_DISABLED ON CACHE BOOL "Disable daScript live host module" FORCE)
    set(DAS_MINFFT_DISABLED ON CACHE BOOL "Disable daScript minfft module" FORCE)
    set(DAS_PUGIXML_DISABLED ON CACHE BOOL "Disable daScript pugixml module" FORCE)
    set(DAS_STBIMAGE_DISABLED ON CACHE BOOL "Disable daScript stbimage module" FORCE)
    set(DAS_STDDLG_DISABLED ON CACHE BOOL "Disable daScript standard dialog module" FORCE)
    set(DAS_TESTS_DISABLED ON CACHE BOOL "Disable daScript tests" FORCE)
    set(DAS_TOOLS_DISABLED ON CACHE BOOL "Disable daScript tools" FORCE)
    set(DAS_TREE_SITTER_DISABLED ON CACHE BOOL "Disable daScript tree-sitter grammar" FORCE)
    set(DAS_TUTORIAL_DISABLED ON CACHE BOOL "Disable daScript tutorial targets" FORCE)

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
