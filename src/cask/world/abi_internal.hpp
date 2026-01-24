#pragma once

#include <cask/abi.h>
#include <cask/world/world.hpp>

inline World* world_from_handle(WorldHandle handle) {
    return static_cast<World*>(handle.world);
}

inline WorldHandle handle_from_world(World* world) {
    return WorldHandle{world};
}
