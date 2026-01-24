#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/plugin/loader.hpp>
#include <cask/plugin/registry.hpp>
#include <cask/world/world.hpp>
#include <string>
#include <vector>

#if defined(__APPLE__)
static const std::string TEST_PLUGIN_PATH = std::string(TEST_PLUGIN_DIR) + "/minimal_plugin.dylib";
#elif defined(__linux__)
static const std::string TEST_PLUGIN_PATH = std::string(TEST_PLUGIN_DIR) + "/minimal_plugin.so";
#elif defined(_WIN32)
static const std::string TEST_PLUGIN_PATH = std::string(TEST_PLUGIN_DIR) + "/minimal_plugin.dll";
#endif

SCENARIO("loading plugins from shared libraries", "[loader]") {
    GIVEN("a plugin at a known path") {
        PluginInfo fake_info{};
        fake_info.name = "TestPlugin";
        
        auto return_fake_info = [&fake_info](const char*) -> LoadResult {
            return {nullptr, &fake_info};
        };
        
        Loader loader(return_fake_info);
        
        WHEN("loading the plugin") {
            LoadResult result = loader.load("./plugins/test.so");
            
            THEN("the plugin info is returned") {
                REQUIRE(result.info != nullptr);
                REQUIRE(std::string(result.info->name) == "TestPlugin");
            }
        }
    }
    
    GIVEN("multiple plugins at different paths") {
        PluginInfo info_a{};
        info_a.name = "PluginA";
        
        PluginInfo info_b{};
        info_b.name = "PluginB";
        
        auto fake_load = [&](const char* path) -> LoadResult {
            if (std::string(path) == "./a.so") return {nullptr, &info_a};
            if (std::string(path) == "./b.so") return {nullptr, &info_b};
            return {nullptr, nullptr};
        };
        
        Loader loader(fake_load);
        
        WHEN("loading all plugins") {
            std::vector<const char*> paths = {"./a.so", "./b.so"};
            auto loaded = loader.load_all(paths);
            
            THEN("all plugin infos are returned") {
                REQUIRE(loaded.size() == 2);
                REQUIRE(std::string(loaded[0].info->name) == "PluginA");
                REQUIRE(std::string(loaded[1].info->name) == "PluginB");
            }
        }
    }
}

SCENARIO("loader tracks handles for cleanup", "[loader]") {
    GIVEN("a loading strategy that provides handles") {
        void* fake_handle = (void*)0x1234;
        PluginInfo fake_info{};
        fake_info.name = "TestPlugin";

        auto fake_strategy = [&](const char*) -> LoadResult {
            return {fake_handle, &fake_info};
        };

        Loader loader(fake_strategy);

        WHEN("loading a plugin") {
            auto result = loader.load("./test.so");

            THEN("both handle and info are available") {
                REQUIRE(result.handle == fake_handle);
                REQUIRE(result.info->name == std::string("TestPlugin"));
            }
        }
    }
}

#if defined(__APPLE__)

SCENARIO("loading a real plugin on macOS", "[loader][integration][macos]") {
    GIVEN("a compiled plugin shared library") {
        Loader loader(macos_strategy);
        
        WHEN("loading the minimal test plugin") {
            auto result = loader.load(TEST_PLUGIN_PATH.c_str());
            
            THEN("the plugin info is returned") {
                REQUIRE(result.handle != nullptr);
                REQUIRE(result.info != nullptr);
                REQUIRE(std::string(result.info->name) == "TimingPlugin");
            }
            
            macos_unload(result.handle);
        }
    }
}

#elif defined(__linux__)

SCENARIO("loading a real plugin on Linux", "[loader][integration][linux]") {
    GIVEN("a compiled plugin shared library") {
        Loader loader(linux_strategy);
        
        WHEN("loading the minimal test plugin") {
            auto result = loader.load(TEST_PLUGIN_PATH.c_str());
            
            THEN("the plugin info is returned") {
                REQUIRE(result.handle != nullptr);
                REQUIRE(result.info != nullptr);
                REQUIRE(std::string(result.info->name) == "TimingPlugin");
            }
            
            linux_unload(result.handle);
        }
    }
}

#elif defined(_WIN32)

SCENARIO("loading a real plugin on Windows", "[loader][integration][windows]") {
    GIVEN("a compiled plugin shared library") {
        Loader loader(windows_strategy);
        
        WHEN("loading the minimal test plugin") {
            auto result = loader.load(TEST_PLUGIN_PATH.c_str());
            
            THEN("the plugin info is returned") {
                REQUIRE(result.handle != nullptr);
                REQUIRE(result.info != nullptr);
                REQUIRE(std::string(result.info->name) == "TimingPlugin");
            }
            
            windows_unload(result.handle);
        }
    }
}

#endif

SCENARIO("native_strategy dispatches to platform loader", "[loader][integration][native]") {
    GIVEN("the native loading strategy") {
        Loader loader(native_strategy);
        
        WHEN("loading the minimal test plugin") {
            auto result = loader.load(TEST_PLUGIN_PATH.c_str());
            
            THEN("the plugin info is returned") {
                REQUIRE(result.handle != nullptr);
                REQUIRE(result.info != nullptr);
                REQUIRE(std::string(result.info->name) == "TimingPlugin");
            }
            
            native_unload(result.handle);
        }
    }
}


