#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WorldHandle WorldHandle;

typedef void (*PluginInitFn)(WorldHandle* handle);
typedef void (*PluginTickFn)(WorldHandle* handle);
typedef void (*PluginFrameFn)(WorldHandle* handle, float alpha);
typedef void (*PluginShutdownFn)(WorldHandle* handle);

typedef struct {
    const char* name;
    const char** defines_components;
    const char** requires_components;
    size_t defines_count;
    size_t requires_count;
    PluginInitFn init_fn;
    PluginTickFn tick_fn;
    PluginFrameFn frame_fn;
    PluginShutdownFn shutdown_fn;
} PluginInfo;

uint32_t world_register_component(WorldHandle* handle, const char* name);
void world_bind(WorldHandle* handle, uint32_t component_id, void* data);
void* world_get_component(WorldHandle* handle, uint32_t component_id);

#ifdef __cplusplus
}
#endif
