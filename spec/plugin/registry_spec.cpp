#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/plugin/registry.hpp>
#include <cask/world/abi_internal.hpp>
#include <string>
#include <vector>

static std::vector<std::string>* init_tracker = nullptr;
static std::vector<std::string>* shutdown_tracker = nullptr;

void track_plugin_a(WorldHandle handle) { init_tracker->push_back("PluginA"); }
void track_plugin_b(WorldHandle handle) { init_tracker->push_back("PluginB"); }

void shutdown_plugin_a(WorldHandle handle) { shutdown_tracker->push_back("PluginA"); }
void shutdown_plugin_b(WorldHandle handle) { shutdown_tracker->push_back("PluginB"); }

static bool* deleter_called_flag = nullptr;

void delete_and_flag(void* data) {
    *deleter_called_flag = true;
    delete static_cast<int*>(data);
}

void init_with_component(WorldHandle handle) {
    int* value = new int(42);
    world_register_and_bind(handle, "ComponentA", value, delete_and_flag);
}

static bool* component_alive_during_shutdown = nullptr;

void shutdown_and_read_component(WorldHandle handle) {
    void* resolved = world_resolve_component(handle, "ComponentA");
    *component_alive_during_shutdown = (resolved != nullptr);
}

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

SCENARIO("plugin registry destroys defined components on shutdown", "[registry]") {
    GIVEN("a plugin that defines a component with a deleter") {
        bool deleter_was_called = false;
        deleter_called_flag = &deleter_was_called;

        static const char* defines[] = {"ComponentA"};
        PluginInfo provider = {
            .name = "Provider",
            .defines_components = defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = init_with_component,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        PluginRegistry registry;
        World world;

        registry.add(&provider);
        registry.initialize(world);

        WHEN("the registry shuts down") {
            registry.shutdown(world);

            THEN("the component deleter is called") {
                REQUIRE(deleter_was_called);
            }
        }
    }

    GIVEN("a plugin whose shutdown_fn reads its own component") {
        bool deleter_was_called = false;
        deleter_called_flag = &deleter_was_called;
        bool was_alive_during_shutdown = false;
        component_alive_during_shutdown = &was_alive_during_shutdown;

        static const char* defines[] = {"ComponentA"};
        PluginInfo provider = {
            .name = "Provider",
            .defines_components = defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = init_with_component,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = shutdown_and_read_component
        };

        PluginRegistry registry;
        World world;

        registry.add(&provider);
        registry.initialize(world);

        WHEN("the registry shuts down") {
            registry.shutdown(world);

            THEN("the shutdown_fn sees the component alive and the deleter runs afterward") {
                REQUIRE(was_alive_during_shutdown);
                REQUIRE(deleter_was_called);
            }
        }
    }
}
