set(SOLDANK_EMSCRIPTEN_ROOT_CANDIDATES)

if(DEFINED ENV{EMSCRIPTEN_ROOT})
    list(APPEND SOLDANK_EMSCRIPTEN_ROOT_CANDIDATES "$ENV{EMSCRIPTEN_ROOT}")
endif()

if(DEFINED ENV{EMSDK})
    list(APPEND SOLDANK_EMSCRIPTEN_ROOT_CANDIDATES "$ENV{EMSDK}/upstream/emscripten")
endif()

find_program(SOLDANK_EMCC_PROGRAM emcc)
if(SOLDANK_EMCC_PROGRAM)
    get_filename_component(SOLDANK_EMCC_REALPATH "${SOLDANK_EMCC_PROGRAM}" REALPATH)
    get_filename_component(SOLDANK_EMCC_DIRECTORY "${SOLDANK_EMCC_REALPATH}" DIRECTORY)
    list(APPEND SOLDANK_EMSCRIPTEN_ROOT_CANDIDATES
        "${SOLDANK_EMCC_DIRECTORY}"
        "${SOLDANK_EMCC_DIRECTORY}/.."
        "${SOLDANK_EMCC_DIRECTORY}/../upstream/emscripten"
    )
endif()

foreach(SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE IN LISTS SOLDANK_EMSCRIPTEN_ROOT_CANDIDATES)
    get_filename_component(SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE
        "${SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE}" ABSOLUTE)
    if(EXISTS "${SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE}/cmake/Modules/Platform/Emscripten.cmake")
        set(SOLDANK_EMSCRIPTEN_ROOT "${SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE}")
        set(SOLDANK_EMSCRIPTEN_ROOT "${SOLDANK_EMSCRIPTEN_ROOT_CANDIDATE}" CACHE PATH
            "Detected Emscripten root")
        break()
    endif()
endforeach()

if(NOT SOLDANK_EMSCRIPTEN_ROOT)
    message(FATAL_ERROR
        "Emscripten.cmake toolchain file not found. "
        "Install/activate emsdk so emcc is in PATH, or set EMSCRIPTEN_ROOT to the Emscripten root."
    )
endif()

include("${SOLDANK_EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
