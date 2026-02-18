#include <catch2/catch_all.hpp>
#include <cstdint>
#include <cask/world/world.hpp>

struct Vec3 {
    float x, y, z;
};

void dummy_deleter(void*) {}

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

SCENARIO("world component resolution", "[world]") {
    GIVEN("a world") {
        World world;

        WHEN("resolving a registered and bound component") {
            int counter_storage = 0;
            uint32_t id = world.register_component("Counter");
            world.bind(id, &counter_storage);

            void* result = world.resolve("Counter");

            THEN("the bound pointer is returned") {
                REQUIRE(result == &counter_storage);
            }
        }

        WHEN("resolving an unregistered name") {
            void* result = world.resolve("NonExistent");

            THEN("nullptr is returned") {
                REQUIRE(result == nullptr);
            }
        }

        WHEN("resolving a registered but unbound component") {
            world.register_component("Counter");

            void* result = world.resolve("Counter");

            THEN("nullptr is returned") {
                REQUIRE(result == nullptr);
            }
        }
    }
}

static bool deleter_called;
static void flag_deleter(void* ptr) {
    delete static_cast<int*>(ptr);
    deleter_called = true;
}

SCENARIO("world component destruction", "[world]") {
    GIVEN("a world") {
        World world;

        WHEN("destroying a component with a deleter") {
            deleter_called = false;
            int* counter_storage = new int(42);
            world.register_and_bind("Counter", counter_storage, flag_deleter);

            world.destroy("Counter");

            THEN("the deleter is called and the component is unresolvable") {
                REQUIRE(deleter_called == true);
                REQUIRE(world.resolve("Counter") == nullptr);
            }
        }

        WHEN("destroying a component with nullptr deleter") {
            int counter_storage = 0;
            world.register_and_bind("Counter", &counter_storage, nullptr);

            world.destroy("Counter");

            THEN("the component is unresolvable") {
                REQUIRE(world.resolve("Counter") == nullptr);
            }
        }

        WHEN("destroying an unregistered name") {
            THEN("it does not throw or crash") {
                REQUIRE_NOTHROW(world.destroy("NonExistent"));
            }
        }
    }
}

SCENARIO("world component register and bind", "[world]") {
    GIVEN("a world") {
        World world;

        WHEN("calling register_and_bind") {
            int counter_storage = 0;
            world.register_and_bind("Counter", &counter_storage, dummy_deleter);

            THEN("the component is resolvable") {
                void* result = world.resolve("Counter");
                REQUIRE(result == &counter_storage);
            }
        }

        WHEN("calling register_and_bind on an already-registered name") {
            int first_storage = 0;
            int second_storage = 0;
            world.register_and_bind("Counter", &first_storage, dummy_deleter);

            THEN("an exception is thrown") {
                REQUIRE_THROWS_AS(
                    world.register_and_bind("Counter", &second_storage, dummy_deleter),
                    std::runtime_error
                );
            }
        }

        WHEN("calling register_and_bind with nullptr deleter") {
            int counter_storage = 0;
            world.register_and_bind("Counter", &counter_storage, nullptr);

            THEN("the component is resolvable") {
                void* result = world.resolve("Counter");
                REQUIRE(result == &counter_storage);
            }
        }
    }
}
