#include <cask/engine/engine.hpp>
#include <cask/plugin/loader.hpp>
#include <cask/plugin/registry.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <vector>

static bool running = true;

void signal_handler(int signal) {
    running = false;
}

struct RealClock {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    float get_time() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<float>(now - start);
        return duration.count();
    }
};

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin.so> [plugin2.so] ...\n";
        return 1;
    }

    Loader loader(native_strategy);
    PluginRegistry registry;
    std::vector<LoadResult> loaded_plugins;

    for (int i = 1; i < argc; i++) {
        auto result = loader.load(argv[i]);
        loaded_plugins.push_back(result);
        registry.add(result.info);
        std::cout << "Loaded: " << result.info->name << "\n";
    }

    Engine engine;
    RealClock clock;
    const float tick_rate = 60.0f;

    registry.initialize(engine.world());

    for (auto* plugin : registry.plugins()) {
        engine.add_system({plugin->tick_fn, plugin->frame_fn});
    }

    std::cout << "Running... (Ctrl+C to stop)\n";

    while (running) {
        engine.step(clock, tick_rate);
    }

    std::cout << "\nShutting down...\n";

    registry.shutdown(engine.world());

    for (auto& result : loaded_plugins) {
        native_unload(result.handle);
    }

    return 0;
}
