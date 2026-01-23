#pragma once

#include "world.hpp"
#include <optional>
#include <vector>

using TickFn = World(*)(World);
using FrameFn = World(*)(World, float);

struct System {
    TickFn tick_fn = nullptr;
    FrameFn frame_fn = nullptr;

    World tick(World world) {
        return tick_fn(std::move(world));
    }

    World frame(World world, float alpha) {
        return frame_fn(std::move(world), alpha);
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

        if (!start_time_) {
            start_time_ = current_time;
            for (auto& system : systems_) {
                if (system.frame_fn) {
                    world_ = system.frame(std::move(world_), 0.0f);
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
                    world_ = system.tick(std::move(world_));
                }
            }
            tick_count_++;
        }

        float alpha = fractional_ticks - target_tick_count;
        for (auto& system : systems_) {
            if (system.frame_fn) {
                world_ = system.frame(std::move(world_), alpha);
            }
        }
    }
};
