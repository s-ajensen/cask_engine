#include <catch2/catch_test_macros.hpp>
#include "engine.hpp"

struct FakeClock {
    float current_time = 0.0f;
    
    float get_time() {
        return current_time;
    }
};

static uint32_t counter_id;
static int* counter_storage;

World increment_counter(World world) {
    int* counter = world.get<int>(counter_id);
    (*counter)++;
    return world;
}

static uint32_t alpha_capture_id;
static float* alpha_storage;
static int frame_call_count;

World capture_alpha(World world, float alpha) {
    float* captured = world.get<float>(alpha_capture_id);
    *captured = alpha;
    frame_call_count++;
    return world;
}

SCENARIO("engine executes frame systems once per step with alpha", "[engine]") {
    GIVEN("an engine with a frame system that captures alpha") {
        FakeClock clock;
        Engine engine;
        
        float alpha_data = -1.0f;
        alpha_storage = &alpha_data;
        alpha_capture_id = engine.world().register_component("AlphaCapture");
        engine.world().bind(alpha_capture_id, alpha_storage);
        frame_call_count = 0;
        
        System frame_system;
        frame_system.frame_fn = capture_alpha;
        engine.add_system(frame_system);
        
        WHEN("time advances partially through a tick") {
            clock.current_time = 0.0f;
            engine.step(clock, 10.0f);
            
            clock.current_time = 0.25f;
            engine.step(clock, 10.0f);
            
            THEN("frame function is called once per step") {
                REQUIRE(frame_call_count == 2);
            }
            
            THEN("frame function receives correct alpha") {
                REQUIRE(*alpha_storage == 0.5f);
            }
        }
    }
}

SCENARIO("engine executes tick systems on fixed timestep", "[engine]") {
    GIVEN("an engine with a system that increments a counter component") {
        FakeClock clock;
        Engine engine;
        
        int counter_data = 0;
        counter_storage = &counter_data;
        counter_id = engine.world().register_component("Counter");
        engine.world().bind(counter_id, counter_storage);
        
        engine.add_system(System{increment_counter});
        
        WHEN("one tick's worth of time elapses") {
            clock.current_time = 0.0f;
            engine.step(clock, 1.0f);
            
            clock.current_time = 1.0f;
            engine.step(clock, 1.0f);
            
            THEN("the system tick function is called exactly once") {
                REQUIRE(*counter_storage == 1);
            }
        }
        
        WHEN("multiple ticks worth of time accumulates") {
            clock.current_time = 0.0f;
            engine.step(clock, 10.0f);
            
            clock.current_time = 0.35f;
            engine.step(clock, 10.0f);
            
            THEN("the system tick function is called multiple times to catch up") {
                REQUIRE(*counter_storage == 3);
            }
        }
    }
}
