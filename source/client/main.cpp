#ifdef _WIN32
#include <windows.h>
#endif

#include <exception>
#include <vector>

// #include "application/Application.hpp"
import Application;

// TODO: odkomentować
// #ifdef _WIN32
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
// #else
int main(int argc, const char* argv[])
// #endif
{
    std::vector<const char*> cli_parameters(argv, argv + argc);
    Soldank::Application app(cli_parameters);
    app.Run();

    return 0;
}
