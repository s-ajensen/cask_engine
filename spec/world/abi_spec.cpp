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

SCENARIO("world_create returns a valid handle", "[abi]") {
    GIVEN("nothing") {
        WHEN("calling world_create") {
            WorldHandle handle = world_create();

            THEN("the handle contains a non-null world pointer") {
                REQUIRE(handle.world != nullptr);
            }

            world_destroy(handle);
        }
    }
}

SCENARIO("created world supports component registration", "[abi]") {
    GIVEN("a world created via world_create") {
        WorldHandle handle = world_create();

        WHEN("registering a component") {
            uint32_t component_id = world_register_component(handle, "Counter");

            THEN("a valid component ID is returned") {
                REQUIRE(component_id != UINT32_MAX);
            }
        }

        world_destroy(handle);
    }
}

SCENARIO("created world supports register_and_bind", "[abi]") {
    GIVEN("a world created via world_create") {
        WorldHandle handle = world_create();

        WHEN("registering, binding, and resolving a component") {
            int storage = 42;
            world_register_and_bind(handle, "Counter", &storage, nullptr);
            void* resolved = world_resolve_component(handle, "Counter");

            THEN("the resolved pointer matches the bound data") {
                REQUIRE(resolved == &storage);
                REQUIRE(*static_cast<int*>(resolved) == 42);
            }
        }

        world_destroy(handle);
    }
}

SCENARIO("world_destroy cleans up without crashing", "[abi]") {
    GIVEN("a world created via world_create") {
        WorldHandle handle = world_create();
        world_register_component(handle, "Counter");

        WHEN("destroying the world") {
            THEN("no crash occurs") {
                REQUIRE_NOTHROW(world_destroy(handle));
            }
        }
    }
}

SCENARIO("multiple worlds are independent", "[abi]") {
    GIVEN("two worlds created via world_create") {
        WorldHandle first_handle = world_create();
        WorldHandle second_handle = world_create();

        WHEN("registering and binding different values in each") {
            int first_value = 111;
            int second_value = 222;
            world_register_and_bind(first_handle, "Counter", &first_value, nullptr);
            world_register_and_bind(second_handle, "Counter", &second_value, nullptr);

            THEN("each world resolves to its own value") {
                int* first_resolved = static_cast<int*>(world_resolve_component(first_handle, "Counter"));
                int* second_resolved = static_cast<int*>(world_resolve_component(second_handle, "Counter"));
                REQUIRE(*first_resolved == 111);
                REQUIRE(*second_resolved == 222);
                REQUIRE(first_resolved != second_resolved);
            }
        }

        world_destroy(first_handle);
        world_destroy(second_handle);
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

SCENARIO("WorldView static create factory", "[worldview]") {
    GIVEN("nothing") {
        WHEN("calling WorldView::create") {
            cask::WorldView world_view = cask::WorldView::create();

            THEN("the handle is valid") {
                REQUIRE(world_view.handle().world != nullptr);
            }

            THEN("the created world supports component operations") {
                int* counter = world_view.register_component<int>("Counter");
                *counter = 99;
                REQUIRE(*world_view.resolve<int>("Counter") == 99);
            }

            world_view.destroy();
        }
    }
}

SCENARIO("WorldView::handle exposes the raw handle", "[worldview]") {
    GIVEN("a WorldView wrapping a known world") {
        World world;
        WorldHandle handle = handle_from_world(&world);
        cask::WorldView world_view(handle);

        WHEN("calling handle()") {
            WorldHandle exposed = world_view.handle();

            THEN("it returns the same handle") {
                REQUIRE(exposed.world == handle.world);
            }
        }
    }
}
