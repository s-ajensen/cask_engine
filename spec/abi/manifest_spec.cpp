#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cask/world/abi_internal.hpp>
#include <cask/abi.h>

static int init_call_count = 0;
static int tick_call_count = 0;
static int shutdown_call_count = 0;
static float last_alpha = -1.0f;

WorldHandle test_init(WorldHandle handle) {
    init_call_count++;
    return handle;
}

WorldHandle test_tick(WorldHandle handle) {
    tick_call_count++;
    return handle;
}

WorldHandle test_frame(WorldHandle handle, float alpha) {
    last_alpha = alpha;
    return handle;
}

WorldHandle test_shutdown(WorldHandle handle) {
    shutdown_call_count++;
    return handle;
}

SCENARIO("PluginInfo defines plugin manifest", "[manifest]") {
    GIVEN("a plugin info struct") {
        static const char* defines[] = {"Position", "Velocity"};
        static const char* requires_list[] = {"Transform"};
        
        PluginInfo info = {
            .name = "TestPlugin",
            .defines_components = defines,
            .requires_components = requires_list,
            .defines_count = 2,
            .requires_count = 1,
            .init_fn = test_init,
            .tick_fn = test_tick,
            .frame_fn = test_frame
        };
        
        THEN("manifest data is accessible") {
            REQUIRE(std::string(info.name) == "TestPlugin");
            REQUIRE(info.defines_count == 2);
            REQUIRE(std::string(info.defines_components[0]) == "Position");
            REQUIRE(std::string(info.defines_components[1]) == "Velocity");
            REQUIRE(info.requires_count == 1);
            REQUIRE(std::string(info.requires_components[0]) == "Transform");
        }
        
        WHEN("calling plugin functions through the manifest") {
            World world;
            WorldHandle handle = handle_from_world(&world);
            
            init_call_count = 0;
            tick_call_count = 0;
            last_alpha = -1.0f;
            
            info.init_fn(handle);
            info.tick_fn(handle);
            info.frame_fn(handle, 0.75f);
            
            THEN("functions are invoked correctly") {
                REQUIRE(init_call_count == 1);
                REQUIRE(tick_call_count == 1);
                REQUIRE(last_alpha == 0.75f);
            }
        }
    }
}

SCENARIO("PluginInfo supports shutdown callback", "[manifest]") {
    GIVEN("a plugin info with shutdown function") {
        PluginInfo info = {
            .name = "ShutdownTestPlugin",
            .defines_components = nullptr,
            .requires_components = nullptr,
            .defines_count = 0,
            .requires_count = 0,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = test_shutdown
        };

        WHEN("calling the shutdown function") {
            World world;
            WorldHandle handle = handle_from_world(&world);
            shutdown_call_count = 0;

            info.shutdown_fn(handle);

            THEN("the shutdown function is invoked") {
                REQUIRE(shutdown_call_count == 1);
            }
        }
    }
}
