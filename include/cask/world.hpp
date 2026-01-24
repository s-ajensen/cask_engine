#pragma once

#include <cask/abi.h>

namespace cask {

class WorldView {
    WorldHandle handle_;

public:
    explicit WorldView(WorldHandle handle) : handle_(handle) {}

    uint32_t register_component(const char* name) {
        return world_register_component(handle_, name);
    }

    void bind(uint32_t component_id, void* data) {
        world_bind(handle_, component_id, data);
    }

    template<typename T>
    T* get(uint32_t component_id) {
        return static_cast<T*>(world_get_component(handle_, component_id));
    }
};

}  // namespace cask
