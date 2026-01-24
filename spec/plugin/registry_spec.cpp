#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/plugin/registry.hpp>
#include <cask/world/abi_internal.hpp>
#include <string>
#include <vector>

static std::vector<std::string>* init_tracker = nullptr;
static std::vector<std::string>* shutdown_tracker = nullptr;

void track_plugin_a(WorldHandle*) { init_tracker->push_back("PluginA"); }
void track_plugin_b(WorldHandle*) { init_tracker->push_back("PluginB"); }

void shutdown_plugin_a(WorldHandle*) { shutdown_tracker->push_back("PluginA"); }
void shutdown_plugin_b(WorldHandle*) { shutdown_tracker->push_back("PluginB"); }

SCENARIO("plugin registry initializes plugins in dependency order", "[registry]") {
    GIVEN("plugins with dependencies added in wrong order") {
        std::vector<std::string> init_order;
        init_tracker = &init_order;

        static const char* plugin_a_defines[] = {"ComponentA"};
        PluginInfo plugin_a = {
            .name = "PluginA",
            .defines_components = plugin_a_defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = track_plugin_a,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        static const char* plugin_b_requires[] = {"ComponentA"};
        PluginInfo plugin_b = {
            .name = "PluginB",
            .defines_components = nullptr,
            .requires_components = plugin_b_requires,
            .defines_count = 0,
            .requires_count = 1,
            .init_fn = track_plugin_b,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        WHEN("initializing with the consumer listed before the provider") {
            PluginRegistry registry;
            World world;

            registry.add(&plugin_b);
            registry.add(&plugin_a);
            registry.initialize(world);

            THEN("the provider is initialized before the consumer") {
                REQUIRE(init_order.size() == 2);
                REQUIRE(init_order[0] == "PluginA");
                REQUIRE(init_order[1] == "PluginB");
            }
        }
    }
}

SCENARIO("plugin registry shuts down in reverse order", "[registry]") {
    GIVEN("initialized plugins with shutdown functions") {
        std::vector<std::string> init_order;
        std::vector<std::string> shutdown_order;
        init_tracker = &init_order;
        shutdown_tracker = &shutdown_order;

        static const char* plugin_a_defines[] = {"ComponentA"};
        PluginInfo plugin_a = {
            .name = "PluginA",
            .defines_components = plugin_a_defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = track_plugin_a,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = shutdown_plugin_a
        };

        static const char* plugin_b_requires[] = {"ComponentA"};
        PluginInfo plugin_b = {
            .name = "PluginB",
            .defines_components = nullptr,
            .requires_components = plugin_b_requires,
            .defines_count = 0,
            .requires_count = 1,
            .init_fn = track_plugin_b,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = shutdown_plugin_b
        };

        PluginRegistry registry;
        World world;

        registry.add(&plugin_b);
        registry.add(&plugin_a);
        registry.initialize(world);

        WHEN("shutting down") {
            registry.shutdown(world);

            THEN("plugins are shut down in reverse init order") {
                REQUIRE(shutdown_order.size() == 2);
                REQUIRE(shutdown_order[0] == "PluginB");
                REQUIRE(shutdown_order[1] == "PluginA");
            }
        }
    }
}
