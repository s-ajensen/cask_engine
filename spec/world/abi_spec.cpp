#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cask/world/abi_internal.hpp>  // Internal: for tests/engine

SCENARIO("C ABI functions bridge to World", "[abi]") {
    GIVEN("a world wrapped in a handle") {
        World world;
        WorldHandle handle = handle_from_world(&world);
        
        WHEN("registering via C ABI") {
            uint32_t id = world_register_component(handle, "Counter");
            
            THEN("component is registered in underlying World") {
                REQUIRE(world.register_component("Counter") == id);
            }
        }
        
        WHEN("binding and getting via C ABI") {
            int storage = 42;
            uint32_t id = world_register_component(handle, "Counter");
            world_bind(handle, id, &storage);
            
            THEN("data is accessible via C ABI") {
                REQUIRE(world_get_component(handle, id) == &storage);
            }
            
            THEN("data is accessible via World directly") {
                REQUIRE(world.get<int>(id) == &storage);
            }
        }
    }
}
