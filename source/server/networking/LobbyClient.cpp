module;

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <utility>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <thread>

export module Networking.LobbyClient;

import Extern.Httplib;
import Extern.Spdlog;
import Shared.Core.Data.FileReader;

export namespace Soldank
{
class LobbyClient
{
public:
    LobbyClient()
    {
        std::string file_path = "lobby_servers.txt";
        FileReader file_reader;
        auto file_data = file_reader.Read("lobby_servers.txt");
        if (!file_data.has_value()) {
            Spdlog::critical("Could not open lobby servers file: {}", file_path);
        } else {
            std::stringstream data_buffer{ *file_data };
            std::string protocol;
            std::string scheme_host_port;
            std::string api_endpoint;
            while (data_buffer >> protocol >> scheme_host_port >> api_endpoint) {
                std::transform(protocol.begin(),
                               protocol.end(),
                               protocol.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                if (api_endpoint.ends_with("/")) {
                    api_endpoint.pop_back();
                }
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                if (protocol == "https") {
                    protocol = "https://";
                } else {
#endif
                    protocol = "http://";
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                }
#endif
                http_clients_.push_back(
                  { Httplib::Client(std::string{ protocol }.append(scheme_host_port)),
                    api_endpoint });
                http_clients_.back().sender.set_follow_location(true);
                http_clients_.back().sender.set_keep_alive(false);
            }
        }
    }

    void Register(const std::string& server_name, std::uint16_t server_port)
    {
#ifdef _WIN32
        std::string operating_system = "Windows";
#elif defined __linux__
        std::string operating_system = "Linux";
#elif defined __APPLE__
        std::string operating_system = "MacOS";
#elif defined __unix__
        std::string operating_system = "UNIX";
#else
        std::string operating_system = "UNKNOWN";
#endif

        // TODO: populate with proper settings
        std::string server_info = "{";

        server_info += R"("advanced": false,)";
        server_info += R"("anti_cheat_on": false,)";
        server_info += R"("bonus_frequency": 0,)";
        server_info += R"("country": PL,)";
        server_info += R"("current_map": "ctf_Ash",)";
        server_info += R"("game_style": "CTF",)";
        server_info += R"("info": "Test info",)";
        server_info += R"("max_players": 12,)";
        server_info += R"("name": "server_name",)";
        server_info += R"("num_bots": 1,)";
        server_info += R"("os": ")" + operating_system + R"(",)";
        server_info += R"("players": [],)";
        server_info += R"("port": ")" + std::to_string(server_port) + ",)";
        server_info += R"("private": false,)";
        server_info += R"("realistic": false,)";
        server_info += R"("respawn": 180,)";
        server_info += R"("survival": false,)";
        server_info += R"("version": "1",)";
        server_info += R"("wm": false")";

        server_info += "}";

        for (auto& http_client : http_clients_) {
            auto response = http_client.sender.Post(
              http_client.api_endpoint + "/servers", server_info, "application/json");
            if (response) {
                if (response->status == 201) {
                    Spdlog::info("Registering in the lobby: {}", http_client.sender.host());
                } else {
                    Spdlog::error("Failed registering in the lobby: {}. Returned status = {}",
                                  http_client.sender.host(),
                                  response->status);
                }
            } else {
                Spdlog::error("Could not register the server in the lobby. Error ({}): {}",
                              response.error_code(),
                              response.error_message());
            }
        }
    }

private:
    struct HTTPClient
    {
        Httplib::Client sender;
        std::string api_endpoint;
    };

    std::vector<HTTPClient> http_clients_;
};
} // namespace Soldank
