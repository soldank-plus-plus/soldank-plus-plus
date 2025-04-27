cmake_minimum_required (VERSION 3.20)

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake )
	message(STATUS "Found ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake; using it!")
	set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
		CACHE STRING "Vcpkg toolchain file")
endif()

project (Soldank++)

option(BUILD_SERVER_ENABLED "Build server" ON)
option(BUILD_CLIENT_ENABLED "Build client" ON)
option(BUILD_TESTS_ENABLED "Build all available tests" ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")

find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

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
    find_package(nlohmann_json CONFIG REQUIRED)
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
    CPMAddPackage(
        NAME dascript
        OPTIONS 
            "DAS_TOOLS_DISABLED ON"
            "DAS_TESTS_DISABLED ON"
            "DAS_PROFILE_DISABLED ON"
            "DAS_TUTORIAL_DISABLED ON"
        URL https://github.com/GaijinEntertainment/daScript/archive/9ae1c4cd73d7da06084aedb5741bb1da68b63b88.tar.gz
        EXCLUDE_FROM_ALL ON
    )
endif()

add_subdirectory(extern)
add_subdirectory(source)
if (BUILD_TESTS_ENABLED)
    enable_testing()
    add_subdirectory(tests)
endif()

