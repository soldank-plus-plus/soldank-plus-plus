module;
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
export module MovementScenarios.Player;
import MovementScenarios.Export;
import MovementScenarios.Catalog;
import Application.Window;
import ClientState;
import Scene;
export namespace Soldank::MovementScenarios
{
int RunPlayer(int argc, char** argv)
{
    auto scenarios = MakeScenarios();
    int selected = 0;
    for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
        if (scenarios[i].id == ScenarioId::LateBackflipLeft) {
            selected = i;
            break;
        }
    }
    std::string export_path, load_path;
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--list-scenarios") {
            for (const auto& scenario : scenarios) {
                std::cout << ScenarioIdToString(scenario.id) << '\n';
            }
            return 0;
        }
        if (argument == "--headless") {
            headless = true;
            continue;
        }
        if ((argument == "--scenario" || argument == "--export" || argument == "--load") &&
            i + 1 < argc) {
            const std::string value = argv[++i];
            if (argument == "--export") {
                export_path = value;
            } else if (argument == "--load") {
                load_path = value;
            } else {
                const auto id = ScenarioIdFromString(value);
                auto found = std::find_if(scenarios.begin(),
                                          scenarios.end(),
                                          [&](const auto& scenario) { return scenario.id == id; });
                if (found == scenarios.end()) {
                    throw std::invalid_argument("Unknown scenario: " + value);
                }
                selected = static_cast<int>(found - scenarios.begin());
            }
        } else {
            throw std::invalid_argument(
              "Usage: --scenario ID --load FILE --export FILE --headless --list-scenarios");
        }
    }
    if (!load_path.empty()) {
        scenarios.push_back(LoadScenario(load_path));
        selected = static_cast<int>(scenarios.size() - 1);
    }
    if (headless) {
        ScenarioRunner runner(scenarios[selected]);
        runner.RunToCompletion();
        if (!export_path.empty()) {
            ExportScenario(runner, export_path);
            ExportCsv(runner, export_path + ".csv");
        }
        return 0;
    }
    Window window("Movement scenarios", 1280, 720);
    window.Create(WindowSizeMode::Windowed);
    window.SetCursorMode(CursorMode::Normal);
    std::unique_ptr<ScenarioRunner> runner;
    std::unique_ptr<ClientState> client;
    std::unique_ptr<Scene> scene;
    bool paused = false, follow = scenarios[selected].follow_camera, exported = false;
    unsigned int loops = 0;
    float speed = 1;
    double accumulator = 0;
    std::string error;
    auto previous = std::chrono::steady_clock::now();
    auto reset = [&]() {
        scene.reset();
        client.reset();
        runner.reset();
        runner = std::make_unique<ScenarioRunner>(scenarios[selected]);
        client = std::make_unique<ClientState>();
        client->client_soldier_id = scenarios[selected].soldier_id;
        client->world_render_options.draw_minimap = false;
        client->world_render_options.camera_shake = false;
        client->camera.view.SetZoom(scenarios[selected].camera_zoom);
        scene = std::make_unique<Scene>(runner->GetStateManager(), *client);
        accumulator = 0;
        previous = std::chrono::steady_clock::now();
        error.clear();
    };
    reset();
    bool final_rendered = false;
    while (!window.ShouldClose()) {
        window.PollInput();
        const auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - previous).count();
        previous = now;
        if (!paused && runner->Completed() && final_rendered) {
            reset();
            elapsed = 0;
            ++loops;
            final_rendered = false;
        }
        if (!paused) {
            accumulator += std::min(elapsed, 0.1) * speed;
        }
        try {
            while (!paused && accumulator >= 1.0 / 60.0 && !runner->Completed()) {
                runner->Step();
                accumulator -= 1.0 / 60.0;
            }
        } catch (const std::exception& exception) {
            error = exception.what();
            paused = true;
        }
        const auto size = window.GetWindowSize();
        client->input.window_width = static_cast<float>(size.x);
        client->input.window_height = static_cast<float>(size.y);
        client->camera.view.UpdateWindowDimensions({ size.x, size.y });
        const auto position = follow ? runner->GetStateManager()
                                         ->GetSoldier(scenarios[selected].soldier_id)
                                         .particle.position
                                     : scenarios[selected].camera_position + runner->GetOffset();
        client->camera.position = { position.x, -position.y };
        client->camera.previous_position = client->camera.position;
        scene->RenderScenario(
          *runner->GetStateManager(),
          *client,
          paused || runner->Completed() ? 1.0 : std::clamp(accumulator * 60.0, 0.0, 1.0));
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Movement scenario player");
        int next = selected;
        if (ImGui::BeginCombo("Scenario", ScenarioIdToString(scenarios[selected].id).c_str())) {
            for (int i = 0; i < static_cast<int>(scenarios.size()); ++i) {
                if (ImGui::Selectable(ScenarioIdToString(scenarios[i].id).c_str(), i == selected)) {
                    next = i;
                }
            }
            ImGui::EndCombo();
        }
        bool restart = next != selected;
        selected = next;
        if (ImGui::Button(paused ? "Resume" : "Pause")) {
            paused = !paused;
            accumulator = 0;
        }
        ImGui::SameLine();
        restart = ImGui::Button("Restart") || restart;
        ImGui::SameLine();
        const bool step = ImGui::Button("Step");
        if (restart) {
            reset();
            final_rendered = false;
        }
        if (step) {
            paused = true;
            accumulator = 0;
            try {
                if (runner->Completed()) {
                    reset();
                    ++loops;
                }
                runner->Step();
            } catch (const std::exception& exception) {
                error = exception.what();
            }
        }
        if (ImGui::RadioButton("0.25x", speed == 0.25F)) {
            speed = 0.25F;
            accumulator = 0;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("0.5x", speed == 0.5F)) {
            speed = 0.5F;
            accumulator = 0;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("1x", speed == 1)) {
            speed = 1;
            accumulator = 0;
        }
        ImGui::Checkbox("Follow soldier", &follow);
        ImGui::TextWrapped("%s", scenarios[selected].description.c_str());
        if (ImGui::Button("Zoom in")) {
            client->camera.view.ZoomIn();
        }
        ImGui::SameLine();
        if (ImGui::Button("Zoom out")) {
            client->camera.view.ZoomOut();
        }
        ImGui::Text("Samples %zu / %u | Loop %u | Setup %u",
                    runner->GetTrace().size(),
                    scenarios[selected].duration_ticks,
                    loops,
                    runner->GetSetupTicks());
        if (!runner->GetTrace().empty()) {
            const auto& s = runner->GetTrace().back();
            ImGui::Text("Tick %u | Left %d Right %d Jump %d Jets %d",
                        s.tick,
                        s.input.left,
                        s.input.right,
                        s.input.jump,
                        s.input.jets);
            ImGui::Text("Position %.3f %.3f | Displacement %.3f %.3f",
                        s.position.x,
                        s.position.y,
                        s.displacement.x,
                        s.displacement.y);
            ImGui::Text("Aim %.2f %.2f", s.aim.x, s.aim.y);
            ImGui::Text("Velocity %.3f %.3f | Grounded %d | Fuel %d | Facing %d",
                        s.velocity.x,
                        s.velocity.y,
                        s.grounded,
                        s.fuel,
                        s.direction);
            ImGui::Text("Body %d frame %u speed %d | Legs %d frame %u speed %d",
                        s.body_type,
                        s.body_frame,
                        s.body_speed,
                        s.legs_type,
                        s.legs_frame,
                        s.legs_speed);
        }
        const bool automatic_export = runner->Completed() && !export_path.empty() && !exported;
        if (ImGui::Button("Export JSON + CSV") || automatic_export) {
            try {
                const auto path = export_path.empty()
                                    ? ScenarioIdToString(scenarios[selected].id) + ".json"
                                    : export_path;
                ExportScenario(*runner, path);
                ExportCsv(*runner, path + ".csv");
                exported = true;
            } catch (const std::exception& exception) {
                error = exception.what();
                paused = true;
            }
        }
        if (!error.empty()) {
            ImGui::TextWrapped("%s", error.c_str());
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.SwapBuffers();
        final_rendered = runner->Completed();
    }
    return 0;
}
} // namespace Soldank::MovementScenarios
