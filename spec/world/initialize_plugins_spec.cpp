#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/world/abi_internal.hpp>
#include <string>
#include <vector>

static std::vector<std::string>* init_tracker = nullptr;

void track_single_plugin(WorldHandle handle) { init_tracker->push_back("TrackedPlugin"); }

PluginInfo make_tracked_plugin(const char* name, PluginInitFn init_fn) {
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

static void track_provider(WorldHandle handle) { init_tracker->push_back("Provider"); }
static void track_consumer(WorldHandle handle) { init_tracker->push_back("Consumer"); }

static const char* provider_defines[] = {"ComponentA"};
static const char* consumer_requires[] = {"ComponentA"};

static PluginInfo make_defining_plugin(const char* name, PluginInitFn init_fn, const char** defines, size_t defines_count) {
    return {
        .name = name,
        .defines_components = defines,
        .requires_components = nullptr,
        .defines_count = defines_count,
        .requires_count = 0,
        .init_fn = init_fn,
        .tick_fn = nullptr,
        .frame_fn = nullptr,
        .shutdown_fn = nullptr
    };
}

static PluginInfo make_requiring_plugin(const char* name, PluginInitFn init_fn, const char** required, size_t requires_count) {
    return {
        .name = name,
        .defines_components = nullptr,
        .requires_components = required,
        .defines_count = 0,
        .requires_count = requires_count,
        .init_fn = init_fn,
        .tick_fn = nullptr,
        .frame_fn = nullptr,
        .shutdown_fn = nullptr
    };
}

SCENARIO("initialize_plugins calls init_fn for a single plugin", "[abi]") {
    GIVEN("a world and a single plugin with a tracking init_fn") {
        std::vector<std::string> tracked_inits;
        init_tracker = &tracked_inits;

        World world;
        WorldHandle handle = handle_from_world(&world);

        PluginInfo plugin = make_tracked_plugin("TrackedPlugin", track_single_plugin);
        PluginInfo* plugin_ptr = &plugin;

        WHEN("initialize_plugins is called with that plugin") {
            initialize_plugins(handle, &plugin_ptr, 1);

            THEN("the init_fn was called exactly once") {
                REQUIRE(tracked_inits.size() == 1);
                REQUIRE(tracked_inits[0] == "TrackedPlugin");
            }
        }
    }
}

SCENARIO("initialize_plugins respects dependency ordering", "[abi]") {
    GIVEN("a consumer listed before its provider in the array") {
        std::vector<std::string> tracked_inits;
        init_tracker = &tracked_inits;

        World world;
        WorldHandle handle = handle_from_world(&world);

        PluginInfo consumer = make_requiring_plugin("Consumer", track_consumer, consumer_requires, 1);
        PluginInfo provider = make_defining_plugin("Provider", track_provider, provider_defines, 1);

        PluginInfo* plugins[] = {&consumer, &provider};

        WHEN("initialize_plugins is called") {
            initialize_plugins(handle, plugins, 2);

            THEN("provider is initialized before consumer") {
                REQUIRE(tracked_inits.size() == 2);
                REQUIRE(tracked_inits[0] == "Provider");
                REQUIRE(tracked_inits[1] == "Consumer");
            }
        }
    }
}
