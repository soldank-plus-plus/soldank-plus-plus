# Soldank++
**Homepage:** http://soldankpp.com/

C++23 implementation of [OpenSoldat](https://github.com/opensoldat/opensoldat) game engine.

[![Discord](https://img.shields.io/discord/1224452056245600417.svg?label=Discord&logo=Discord&colorB=7289da&style=for-the-badge)](https://discord.gg/gvhsk8NZHD)

# Screenshot
![screenshot](https://www.dropbox.com/scl/fi/i9yp7clwdossl0j3p3myh/soldank-plus-plus.png?rlkey=oysbty3186yz3jc3mkj2fela9&raw=1)

## Requirements
The project uses some of the features from C++23 standard, such as: ranges::contains or std::format.
Therefore, the minimum versions of the main compilers are:
- GCC libstdc++ 13
- Clang libc++ 15
- Visual Studio 2022 17.6.5

The project is written in C++23 and depends directly on:
- [OpenGL 2.1+](https://www.opengl.org/)
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)
- [glad](https://glad.dav1d.de/)
- [glfw-3.3.8+](https://www.glfw.org/)
- [glm](https://github.com/g-truc/glm)
- [stb](https://github.com/nothings/stb)
- [FreeType](https://freetype.org/)
- [gtest 1.13.0+](https://github.com/google/googletest)
- [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
- [daScript 0.4](https://dascript.org/)
- [cxxopts](https://github.com/jarro2783/cxxopts)
- [spdlog](https://github.com/gabime/spdlog)
- [simpleini](https://github.com/brofield/simpleini)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GoogleTest](https://github.com/google/googletest)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://json.nlohmann.me/)

And indirectly (some of the above packages use those) on:
- [Protobuf](https://github.com/protocolbuffers/protobuf)
- [ZLIB](https://github.com/madler/zlib)
- [libpng](https://github.com/pnggroup/libpng)
- [BZip2](https://sourceware.org/bzip2/)

## Building
Recommended to build using [vcpkg](https://github.com/microsoft/vcpkg) to prepare
the package [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
which unfortunately can't be prepared using [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)

### Windows
In project root directory run:
```
> git clone https://github.com/microsoft/vcpkg
> .\vcpkg\bootstrap-vcpkg.bat
```

Make sure you have CMake installed. The project uses [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) to download required packages.
To install all the required package using:
```
> mkdir build
> cd build
> cmake ..
```
You should now be able to build executables using `make` on linux or to open generated solution in visual studio on windows
