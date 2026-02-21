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

static bool* component_accessible_flag = nullptr;

void init_and_resolve_component(WorldHandle handle) {
    init_tracker->push_back("PluginB");
    void* resolved = world_resolve_component(handle, "ComponentA");
    *component_accessible_flag = (resolved != nullptr);
}

PluginInfo make_plugin(const char* name, PluginInitFn init_fn) {
    return {
        .name = name,
        .defines_components = nullptr,
        .requires_components = nullptr,
        .defines_count = 0,
        .requires_count = 0,
        .init_fn = init_fn,
        .tick_fn = nullptr,
        .frame_fn = nullptr,
        .shutdown_fn = nullptr
    };
}

PluginInfo make_defining_plugin(const char* name, PluginInitFn init_fn, const char** defines, size_t defines_count) {
    PluginInfo plugin = make_plugin(name, init_fn);
    plugin.defines_components = defines;
    plugin.defines_count = defines_count;
    return plugin;
}

PluginInfo make_requiring_plugin(const char* name, PluginInitFn init_fn, const char** deps, size_t deps_count) {
    PluginInfo plugin = make_plugin(name, init_fn);
    plugin.requires_components = deps;
    plugin.requires_count = deps_count;
    return plugin;
}

SCENARIO("plugin registry initializes plugins in dependency order", "[registry]") {
    GIVEN("plugins with dependencies added in wrong order") {
        std::vector<std::string> init_order;
        init_tracker = &init_order;

        static const char* plugin_a_defines[] = {"ComponentA"};
        PluginInfo plugin_a = make_defining_plugin("PluginA", track_plugin_a, plugin_a_defines, 1);

        static const char* plugin_b_requires[] = {"ComponentA"};
        PluginInfo plugin_b = make_requiring_plugin("PluginB", track_plugin_b, plugin_b_requires, 1);

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
        PluginInfo plugin_a = make_defining_plugin("PluginA", track_plugin_a, plugin_a_defines, 1);
        plugin_a.shutdown_fn = shutdown_plugin_a;

        static const char* plugin_b_requires[] = {"ComponentA"};
        PluginInfo plugin_b = make_requiring_plugin("PluginB", track_plugin_b, plugin_b_requires, 1);
        plugin_b.shutdown_fn = shutdown_plugin_b;

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
        PluginInfo provider = make_defining_plugin("Provider", init_with_component, defines, 1);

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
        PluginInfo provider = make_defining_plugin("Provider", init_with_component, defines, 1);
        provider.shutdown_fn = shutdown_and_read_component;

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

SCENARIO("plugin registry ignores duplicate adds", "[registry]") {
    GIVEN("a plugin added to the registry twice") {
        std::vector<std::string> init_order;
        init_tracker = &init_order;

        PluginInfo plugin_a = make_plugin("PluginA", track_plugin_a);

        PluginRegistry registry;
        World world;

        registry.add(&plugin_a);
        registry.add(&plugin_a);

        WHEN("initializing the registry") {
            registry.initialize(world);

            THEN("the plugin init_fn is called exactly once") {
                REQUIRE(init_order.size() == 1);
                REQUIRE(init_order[0] == "PluginA");
            }
        }
    }
}

SCENARIO("plugin registry respects dependencies across incremental loads", "[registry]") {
    GIVEN("a plugin that defines a component already initialized") {
        std::vector<std::string> tracked_inits;
        init_tracker = &tracked_inits;
        bool deleter_was_called = false;
        deleter_called_flag = &deleter_was_called;
        bool component_was_accessible = false;
        component_accessible_flag = &component_was_accessible;

        static const char* plugin_a_defines[] = {"ComponentA"};
        PluginInfo plugin_a = make_defining_plugin("PluginA", init_with_component, plugin_a_defines, 1);

        PluginRegistry registry;
        World world;

        registry.add(&plugin_a);
        registry.initialize(world);

        WHEN("a dependent plugin is added and initialized incrementally") {
            static const char* plugin_b_requires[] = {"ComponentA"};
            PluginInfo plugin_b = make_requiring_plugin("PluginB", init_and_resolve_component, plugin_b_requires, 1);

            registry.add(&plugin_b);
            registry.initialize(world);

            THEN("the new plugin can access the previously initialized component") {
                REQUIRE(component_was_accessible);
            }
        }
    }
}

SCENARIO("plugin registry re-initialize only inits new plugins", "[registry]") {
    GIVEN("a registry with one initialized plugin") {
        std::vector<std::string> tracked_inits;
        init_tracker = &tracked_inits;

        PluginInfo plugin_a = make_plugin("PluginA", track_plugin_a);

        PluginRegistry registry;
        World world;

        registry.add(&plugin_a);
        registry.initialize(world);

        REQUIRE(tracked_inits.size() == 1);
        REQUIRE(tracked_inits[0] == "PluginA");

        tracked_inits.clear();

        WHEN("a new plugin is added and initialize is called again") {
            PluginInfo plugin_b = make_plugin("PluginB", track_plugin_b);

            registry.add(&plugin_b);
            registry.initialize(world);

            THEN("only the new plugin is initialized") {
                REQUIRE(tracked_inits.size() == 1);
                REQUIRE(tracked_inits[0] == "PluginB");
            }
        }
    }
}

SCENARIO("plugin registry initialize returns newly initialized plugins", "[registry]") {
    GIVEN("two standalone plugins added incrementally") {
        std::vector<std::string> tracked_inits;
        init_tracker = &tracked_inits;

        PluginInfo plugin_a = make_plugin("PluginA", track_plugin_a);
        PluginInfo plugin_b = make_plugin("PluginB", track_plugin_b);

        PluginRegistry registry;
        World world;

        WHEN("the first plugin is added and initialized") {
            registry.add(&plugin_a);
            auto first_batch = registry.initialize(world);

            THEN("initialize returns only the first plugin") {
                REQUIRE(first_batch.size() == 1);
                REQUIRE(std::string(first_batch[0]->name) == "PluginA");
            }
        }

        WHEN("a second plugin is added and initialized after the first") {
            registry.add(&plugin_a);
            registry.initialize(world);

            registry.add(&plugin_b);
            auto second_batch = registry.initialize(world);

            THEN("initialize returns only the newly initialized plugin") {
                REQUIRE(second_batch.size() == 1);
                REQUIRE(std::string(second_batch[0]->name) == "PluginB");
            }
        }
    }
}
