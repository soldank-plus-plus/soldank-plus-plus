#include <exception>
#include <iostream>
#include <string>
import MovementScenarios.Player;
int main(int argc, char** argv)
{
    try {
        return Soldank::MovementScenarios::RunPlayer(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const char* error) {
        std::cerr << error << '\n';
        return 1;
    }
}
