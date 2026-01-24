#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cask/abi.h>
#include <cask/deps/graph.hpp>
#include <vector>

SCENARIO("dependency graph resolves plugin init order", "[deps]") {
    GIVEN("a single plugin with no dependencies") {
        PluginInfo plugin = {
            .name = "StandalonePlugin",
            .defines_components = nullptr,
            .requires_components = nullptr,
            .defines_count = 0,
            .requires_count = 0,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        std::vector<PluginInfo*> plugins = {&plugin};

        WHEN("resolving dependencies") {
            auto result = cask::deps::resolve(plugins);

            THEN("the plugin is returned in the init order") {
                REQUIRE(result.size() == 1);
                REQUIRE(result[0] == &plugin);
            }
        }
    }

    GIVEN("a consumer plugin listed before its provider") {
        static const char* provider_defines[] = {"Transform"};
        PluginInfo provider = {
            .name = "ProviderPlugin",
            .defines_components = provider_defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        static const char* consumer_requires[] = {"Transform"};
        PluginInfo consumer = {
            .name = "ConsumerPlugin",
            .defines_components = nullptr,
            .requires_components = consumer_requires,
            .defines_count = 0,
            .requires_count = 1,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        std::vector<PluginInfo*> plugins = {&consumer, &provider};

        WHEN("resolving dependencies") {
            auto result = cask::deps::resolve(plugins);

            THEN("provider is ordered before consumer") {
                REQUIRE(result.size() == 2);
                REQUIRE(result[0] == &provider);
                REQUIRE(result[1] == &consumer);
            }
        }
    }

    GIVEN("a plugin requiring a component that nothing defines") {
        static const char* consumer_requires[] = {"Transform"};
        PluginInfo consumer = {
            .name = "OrphanedConsumer",
            .defines_components = nullptr,
            .requires_components = consumer_requires,
            .defines_count = 0,
            .requires_count = 1,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        std::vector<PluginInfo*> plugins = {&consumer};

        WHEN("resolving dependencies") {
            THEN("an error is thrown mentioning the missing component") {
                REQUIRE_THROWS_WITH(
                    cask::deps::resolve(plugins),
                    Catch::Matchers::ContainsSubstring("Transform")
                );
            }
        }
    }

    GIVEN("two plugins that both define the same component") {
        static const char* defines[] = {"Transform"};

        PluginInfo plugin_a = {
            .name = "PluginA",
            .defines_components = defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        PluginInfo plugin_b = {
            .name = "PluginB",
            .defines_components = defines,
            .requires_components = nullptr,
            .defines_count = 1,
            .requires_count = 0,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        std::vector<PluginInfo*> plugins = {&plugin_a, &plugin_b};

        WHEN("resolving dependencies") {
            THEN("an error is thrown mentioning the duplicate component") {
                REQUIRE_THROWS_WITH(
                    cask::deps::resolve(plugins),
                    Catch::Matchers::ContainsSubstring("Transform")
                );
            }
        }
    }

    GIVEN("two plugins with circular dependencies") {
        static const char* plugin_a_defines[] = {"ComponentA"};
        static const char* plugin_a_requires[] = {"ComponentB"};
        PluginInfo plugin_a = {
            .name = "PluginA",
            .defines_components = plugin_a_defines,
            .requires_components = plugin_a_requires,
            .defines_count = 1,
            .requires_count = 1,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        static const char* plugin_b_defines[] = {"ComponentB"};
        static const char* plugin_b_requires[] = {"ComponentA"};
        PluginInfo plugin_b = {
            .name = "PluginB",
            .defines_components = plugin_b_defines,
            .requires_components = plugin_b_requires,
            .defines_count = 1,
            .requires_count = 1,
            .init_fn = nullptr,
            .tick_fn = nullptr,
            .frame_fn = nullptr,
            .shutdown_fn = nullptr
        };

        std::vector<PluginInfo*> plugins = {&plugin_a, &plugin_b};

        WHEN("resolving dependencies") {
            THEN("an error is thrown listing the plugins in the cycle") {
                REQUIRE_THROWS_WITH(
                    cask::deps::resolve(plugins),
                    Catch::Matchers::ContainsSubstring("circular") &&
                    Catch::Matchers::ContainsSubstring("PluginA") &&
                    Catch::Matchers::ContainsSubstring("PluginB")
                );
            }
        }
    }
}
