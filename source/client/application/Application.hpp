#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "core/math/Glm.hpp"

#include <vector>
#include <memory>

namespace Soldank
{
class Window;
class IWorld;
class INetworkingClient;
class ClientState;
class NetworkEventDispatcher;
class MapEditor;

namespace CommandLineParameters
{
enum class ApplicationMode;
}

enum class WindowSizeMode : std::uint8_t;

class Application
{
public:
    Application(const std::vector<const char*>& cli_parameters);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(Application other) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&& other) = delete;

    void Run();

private:
    glm::vec2 GetCurrentMouseScreenPosition();
    glm::vec2 GetCurrentMouseMapPosition();
    void UpdateWindowSize();

    std::unique_ptr<Window> window_;
    std::shared_ptr<IWorld> world_;
    std::unique_ptr<INetworkingClient> networking_client_;
    std::shared_ptr<ClientState> client_state_;
    std::shared_ptr<NetworkEventDispatcher> client_network_event_dispatcher_;
    std::unique_ptr<MapEditor> map_editor_;

    CommandLineParameters::ApplicationMode application_mode_;
    WindowSizeMode window_size_mode_;

    int fps_limit_ = 0;
};
} // namespace Soldank

#endif
