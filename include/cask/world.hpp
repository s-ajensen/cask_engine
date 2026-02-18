#pragma once

#include <cask/abi.h>
#include <stdexcept>
#include <string>

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

    template<typename T>
    T* resolve(const char* name) {
        void* ptr = world_resolve_component(handle_, name);
        if (!ptr) throw std::runtime_error(std::string("Component not found: ") + name);
        return static_cast<T*>(ptr);
    }

    template<typename T>
    T* register_component(const char* name) {
        T* instance = new T();
        world_register_and_bind(handle_, name, instance, [](void* raw) { delete static_cast<T*>(raw); });
        return instance;
    }
};

}  // namespace cask
