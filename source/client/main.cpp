#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <exception>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// #include "application/Application.hpp"
import Application;

// TODO: odkomentować
// #ifdef _WIN32
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
// #else
int main(int argc, const char* argv[])
// #endif
{
#ifdef __EMSCRIPTEN__
    auto* app = new Soldank::Application(argc, argv);
    app->Run();
    emscripten_exit_with_live_runtime();
#else
    Soldank::Application app(argc, argv);
    app.Run();
#endif

    return 0;
}
