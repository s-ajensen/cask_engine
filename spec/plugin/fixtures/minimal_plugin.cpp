#include <cask/abi.h>

static uint32_t counter_id = 0;
static int counter_value = 0;

WorldHandle plugin_init(WorldHandle handle) {
    counter_id = world_register_component(handle, "Counter");
    world_bind(handle, counter_id, &counter_value);
    return handle;
}

WorldHandle plugin_tick(WorldHandle handle) {
    int* counter = (int*)world_get_component(handle, counter_id);
    (*counter)++;
    return handle;
}

WorldHandle plugin_shutdown(WorldHandle handle) {
    counter_value = 0;
    return handle;
}

static const char* defines_components[] = {"Counter"};

static PluginInfo plugin_info = {
    .name = "CounterPlugin",
    .defines_components = defines_components,
    .requires_components = nullptr,
    .defines_count = 1,
    .requires_count = 0,
    .init_fn = plugin_init,
    .tick_fn = plugin_tick,
    .frame_fn = nullptr,
    .shutdown_fn = plugin_shutdown
};

extern "C" PluginInfo* get_plugin_info() {
    return &plugin_info;
}
