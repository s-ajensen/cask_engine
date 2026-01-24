#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cask/world/world.hpp>

struct Vec3 {
    float x, y, z;
};

SCENARIO("world component registration", "[world]") {
    GIVEN("a world") {
        World world;

        WHEN("registering a component") {
            uint32_t id = world.register_component("Counter");

            THEN("an ID is returned") {
                REQUIRE(id != UINT32_MAX);
            }
        }

        WHEN("registering the same component twice") {
            uint32_t first_id = world.register_component("Counter");
            uint32_t second_id = world.register_component("Counter");

            THEN("the same ID is returned") {
                REQUIRE(first_id == second_id);
            }
        }

        WHEN("registering different components") {
            uint32_t counter_id = world.register_component("Counter");
            uint32_t position_id = world.register_component("Position");

            THEN("different IDs are returned") {
                REQUIRE(counter_id != position_id);
            }
        }

        WHEN("binding and accessing component data") {
            int counter_storage = 0;
            
            uint32_t id = world.register_component("Counter");
            world.bind(id, &counter_storage);

            int* counter = world.get<int>(id);
            *counter = 42;

            THEN("the stored value persists") {
                REQUIRE(counter_storage == 42);
                REQUIRE(*world.get<int>(id) == 42);
            }
        }

        WHEN("binding the same component twice") {
            int storage1 = 0;
            int storage2 = 0;
            
            uint32_t id = world.register_component("Counter");
            world.bind(id, &storage1);

            THEN("an exception is thrown") {
                REQUIRE_THROWS_AS(
                    world.bind(id, &storage2),
                    std::runtime_error
                );
            }
        }
    }
}
