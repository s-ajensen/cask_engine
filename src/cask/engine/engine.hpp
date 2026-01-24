#pragma once

#include <cask/abi.h>
#include <cask/world/world.hpp>
#include <optional>
#include <vector>

using TickFn = WorldHandle(*)(WorldHandle);
using FrameFn = WorldHandle(*)(WorldHandle, float);

struct System {
    TickFn tick_fn = nullptr;
    FrameFn frame_fn = nullptr;

    WorldHandle tick(WorldHandle handle) {
        return tick_fn(handle);
    }

    WorldHandle frame(WorldHandle handle, float alpha) {
        return frame_fn(handle, alpha);
    }
};

class Engine {
    World world_;
    std::vector<System> systems_;
    std::optional<float> start_time_;
    int tick_count_ = 0;

public:
    World& world() {
        return world_;
    }

    void add_system(System system) {
        systems_.push_back(system);
    }

    template<typename Clock>
    void step(Clock& clock, float tick_rate) {
        float current_time = clock.get_time();
        WorldHandle handle = {&world_};

        if (!start_time_) {
            start_time_ = current_time;
            for (auto& system : systems_) {
                if (system.frame_fn) {
                    handle = system.frame(handle, 0.0f);
                }
            }
            return;
        }

        float total_elapsed = current_time - *start_time_;
        float fractional_ticks = total_elapsed * tick_rate;
        int target_tick_count = static_cast<int>(fractional_ticks);

        while (tick_count_ < target_tick_count) {
            for (auto& system : systems_) {
                if (system.tick_fn) {
                    handle = system.tick(handle);
                }
            }
            tick_count_++;
        }

        float alpha = fractional_ticks - target_tick_count;
        for (auto& system : systems_) {
            if (system.frame_fn) {
                handle = system.frame(handle, alpha);
            }
        }
    }
};
