#include <exception>

import Application;

int main()
{
    try {
        Soldank::Application application;
        application.Run();
    } catch (const std::exception& /*exception*/) {
        return 1;
    }

    return 0;
}
