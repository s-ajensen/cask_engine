#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cask/world/abi_internal.hpp>  // Internal: to create WorldHandle in test
#include <cask/world.hpp>               // Public: plugin API

SCENARIO("WorldView provides C++ convenience over C ABI", "[plugin]") {
    GIVEN("a world wrapped in a handle") {
        World world;
        WorldHandle handle{&world};
        
        WHEN("using WorldView to register and bind") {
            cask::WorldView view(&handle);
            
            int storage = 0;
            uint32_t id = view.register_component("Counter");
            view.bind(id, &storage);
            
            THEN("data is accessible via typed get") {
                int* counter = view.get<int>(id);
                REQUIRE(counter == &storage);
            }
        }
        
        WHEN("using WorldView to modify data") {
            cask::WorldView view(&handle);
            
            int storage = 0;
            uint32_t id = view.register_component("Counter");
            view.bind(id, &storage);
            
            int* counter = view.get<int>(id);
            *counter = 42;
            
            THEN("modifications persist") {
                REQUIRE(storage == 42);
                REQUIRE(*view.get<int>(id) == 42);
            }
        }
    }
}
