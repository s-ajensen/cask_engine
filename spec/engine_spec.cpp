#include <catch2/catch_test_macros.hpp>
#include "engine.hpp"

struct FakeClock {
    float current_time = 0.0f;
    
    float get_time() {
        return current_time;
    }
};

static uint32_t counter_id;

World increment_counter(World world) {
    int* counter = static_cast<int*>(world.get_component(counter_id));
    (*counter)++;
    return world;
}

static uint32_t alpha_capture_id;
static int frame_call_count;

World capture_alpha(World world, float alpha) {
    float* captured = static_cast<float*>(world.get_component(alpha_capture_id));
    *captured = alpha;
    frame_call_count++;
    return world;
}

SCENARIO("engine executes frame systems once per step with alpha", "[engine]") {
    GIVEN("an engine with a frame system that captures alpha") {
        FakeClock clock;
        Engine engine;
        
        alpha_capture_id = engine.world().define_component("AlphaCapture", sizeof(float));
        float* captured = static_cast<float*>(engine.world().get_component(alpha_capture_id));
        *captured = -1.0f;
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
                float* result = static_cast<float*>(engine.world().get_component(alpha_capture_id));
                REQUIRE(*result == 0.5f);
            }
        }
    }
}

SCENARIO("engine executes tick systems on fixed timestep", "[engine]") {
    GIVEN("an engine with a system that increments a counter component") {
        FakeClock clock;
        Engine engine;
        
        counter_id = engine.world().define_component("Counter", sizeof(int));
        int* counter = static_cast<int*>(engine.world().get_component(counter_id));
        *counter = 0;
        
        engine.add_system(System{increment_counter});
        
        WHEN("one tick's worth of time elapses") {
            clock.current_time = 0.0f;
            engine.step(clock, 1.0f);
            
            clock.current_time = 1.0f;
            engine.step(clock, 1.0f);
            
            THEN("the system tick function is called exactly once") {
                int* result = static_cast<int*>(engine.world().get_component(counter_id));
                REQUIRE(*result == 1);
            }
        }
        
        WHEN("multiple ticks worth of time accumulates") {
            clock.current_time = 0.0f;
            engine.step(clock, 10.0f);
            
            clock.current_time = 0.35f;
            engine.step(clock, 10.0f);
            
            THEN("the system tick function is called multiple times to catch up") {
                int* result = static_cast<int*>(engine.world().get_component(counter_id));
                REQUIRE(*result == 3);
            }
        }
    }
}
