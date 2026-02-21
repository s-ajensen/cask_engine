#include <cask/world/abi_internal.hpp>

extern "C" {

WorldHandle world_create() {
    return handle_from_world(new World());
}

void world_destroy(WorldHandle handle) {
    delete world_from_handle(handle);
}

uint32_t world_register_component(WorldHandle handle, const char* name) {
    return world_from_handle(handle)->register_component(name);
}

void world_bind(WorldHandle handle, uint32_t component_id, void* data) {
    world_from_handle(handle)->bind(component_id, data);
}

void* world_get_component(WorldHandle handle, uint32_t component_id) {
    return world_from_handle(handle)->get_component(component_id);
}

void* world_resolve_component(WorldHandle handle, const char* name) {
    return world_from_handle(handle)->resolve(name);
}

void world_register_and_bind(WorldHandle handle, const char* name, void* data, ComponentDeleter deleter) {
    world_from_handle(handle)->register_and_bind(name, data, deleter);
}

}
