file(GLOB_RECURSE LIB_SOURCES *.cpp ${CMAKE_SOURCE_DIR}/build/bindings/imgui/*)
file(GLOB_RECURSE HEADER_SOURCES *.hpp)
list(REMOVE_ITEM LIB_SOURCES main.cpp)

add_library(client_lib ${LIB_SOURCES} ${HEADER_SOURCES})

target_compile_definitions(client_lib PUBLIC ${CLIENT_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(client_lib PRIVATE /Wall)
else()
  target_compile_options(client_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(client_lib PUBLIC ${SIMPLEINI_INCLUDE_DIRS})
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client_lib PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
target_include_directories(client_lib PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(client_lib PUBLIC glfw)
target_link_libraries(client_lib PUBLIC glad)
target_link_libraries(client_lib PUBLIC glm::glm)

# TODO: check if PNG, ZLIB and BZip2 are needed for linking here
target_link_libraries(client_lib PRIVATE PNG::PNG)
target_link_libraries(client_lib PRIVATE ZLIB::ZLIB)
target_link_libraries(client_lib PRIVATE BZip2::BZip2)

target_link_libraries(client_lib PUBLIC Freetype::Freetype)
target_link_libraries(client_lib PRIVATE spdlog::spdlog)
target_link_libraries(client_lib PRIVATE imgui::imgui)
target_link_libraries(client_lib PUBLIC shared_lib)
target_link_libraries(client_lib PRIVATE GameNetworkingSockets::GameNetworkingSockets_s)
target_link_libraries(client_lib PRIVATE cxxopts::cxxopts)

# TODO: uncomment this to stop the app from opening cmd on windows
# IF (WIN32)
#    add_executable(client WIN32 main.cpp)
# ELSE()
   add_executable(client main.cpp)
# ENDIF()
target_compile_definitions(client PUBLIC ${CLIENT_COMPILE_DEFINITIONS})

if(MSVC)
  target_compile_options(client PRIVATE /Wall)
  set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
  target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
  set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

target_link_libraries(client PRIVATE client_lib)
target_link_libraries(client PRIVATE GameNetworkingSockets::GameNetworkingSockets_s)
target_link_libraries(client PRIVATE spdlog::spdlog)
target_link_libraries(client PRIVATE cxxopts::cxxopts)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/source/client)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/build/bindings)
install(TARGETS client DESTINATION ${CMAKE_SOURCE_DIR}/bin)
