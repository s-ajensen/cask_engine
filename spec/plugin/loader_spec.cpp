#include <catch2/catch_test_macros.hpp>
#include <cask/abi.h>
#include <cask/plugin/loader.hpp>
#include <vector>

SCENARIO("loading plugins from shared libraries", "[loader]") {
    GIVEN("a plugin at a known path") {
        PluginInfo fake_info{};
        fake_info.name = "TestPlugin";
        
        auto return_fake_info = [&fake_info](const char*) -> PluginInfo* {
            return &fake_info;
        };
        
        Loader loader(return_fake_info);
        
        WHEN("loading the plugin") {
            PluginInfo* info = loader.load("./plugins/test.so");
            
            THEN("the plugin info is returned") {
                REQUIRE(info != nullptr);
                REQUIRE(std::string(info->name) == "TestPlugin");
            }
        }
    }
    
    GIVEN("multiple plugins at different paths") {
        PluginInfo info_a{};
        info_a.name = "PluginA";
        
        PluginInfo info_b{};
        info_b.name = "PluginB";
        
        auto fake_load = [&](const char* path) -> PluginInfo* {
            if (std::string(path) == "./a.so") return &info_a;
            if (std::string(path) == "./b.so") return &info_b;
            return nullptr;
        };
        
        Loader loader(fake_load);
        
        WHEN("loading all plugins") {
            std::vector<const char*> paths = {"./a.so", "./b.so"};
            auto loaded = loader.load_all(paths);
            
            THEN("all plugin infos are returned") {
                REQUIRE(loaded.size() == 2);
                REQUIRE(std::string(loaded[0]->name) == "PluginA");
                REQUIRE(std::string(loaded[1]->name) == "PluginB");
            }
        }
    }
}
