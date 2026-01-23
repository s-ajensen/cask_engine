#include <cask/world/abi_internal.hpp>

extern "C" {

uint32_t world_register_component(WorldHandle* handle, const char* name) {
    return handle->world->register_component(name);
}

void world_bind(WorldHandle* handle, uint32_t component_id, void* data) {
    handle->world->bind(component_id, data);
}

void* world_get_component(WorldHandle* handle, uint32_t component_id) {
    return handle->world->get_component(component_id);
}

}
