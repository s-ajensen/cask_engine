#include <catch2/catch_all.hpp>
#include <cstdint>
#include <stdexcept>
#include <cask/world/abi_internal.hpp>
#include <cask/world.hpp>

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

SCENARIO("C ABI resolve and register_and_bind", "[abi]") {
    GIVEN("a world wrapped in a handle") {
        World world;
        WorldHandle handle = handle_from_world(&world);

        WHEN("resolving a component registered via the old API") {
            int storage = 99;
            uint32_t id = world_register_component(handle, "Counter");
            world_bind(handle, id, &storage);

            THEN("world_resolve_component returns the bound pointer") {
                REQUIRE(world_resolve_component(handle, "Counter") == &storage);
            }

            THEN("world_resolve_component returns nullptr for unknown names") {
                REQUIRE(world_resolve_component(handle, "NonExistent") == nullptr);
            }
        }

        WHEN("using register_and_bind via C ABI") {
            int storage = 77;
            world_register_and_bind(handle, "Counter", &storage, nullptr);

            THEN("the component is accessible via world_resolve_component") {
                REQUIRE(world_resolve_component(handle, "Counter") == &storage);
            }
        }
    }
}

SCENARIO("WorldView convenience templates", "[worldview]") {
    GIVEN("a world wrapped in a WorldView") {
        World world;
        WorldHandle handle = handle_from_world(&world);
        cask::WorldView world_view(handle);

        WHEN("resolving a component registered via the old API") {
            int storage = 42;
            uint32_t component_id = world_register_component(handle, "Counter");
            world_bind(handle, component_id, &storage);

            THEN("resolve returns a typed pointer to the bound data") {
                int* result = world_view.resolve<int>("Counter");
                REQUIRE(result == &storage);
                REQUIRE(*result == 42);
            }
        }

        WHEN("resolving a component that does not exist") {
            THEN("resolve throws std::runtime_error") {
                REQUIRE_THROWS_AS(world_view.resolve<int>("NonExistent"), std::runtime_error);
            }
        }

        WHEN("using register_component to create and bind a component") {
            int* counter = world_view.register_component<int>("Counter");
            *counter = 123;

            THEN("the component is accessible via world_resolve_component") {
                void* raw = world_resolve_component(handle, "Counter");
                REQUIRE(raw == counter);
                REQUIRE(*static_cast<int*>(raw) == 123);
            }
        }
    }
}
