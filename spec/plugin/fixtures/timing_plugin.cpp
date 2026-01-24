#include <cask/abi.h>
#include <cask/world.hpp>
#include <cstdio>

static uint32_t tick_count_id = 0;
static uint32_t frame_count_id = 0;
static int tick_count = 0;
static int frame_count = 0;

WorldHandle plugin_init(WorldHandle handle) {
    cask::WorldView world(handle);
    tick_count_id = world.register_component("TickCount");
    frame_count_id = world.register_component("FrameCount");
    world.bind(tick_count_id, &tick_count);
    world.bind(frame_count_id, &frame_count);
    std::printf("Plugin initialized\n");
    return handle;
}

WorldHandle plugin_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    int* ticks = world.get<int>(tick_count_id);
    (*ticks)++;
    std::printf("[TICK %d]\n", *ticks);
    return handle;
}

WorldHandle plugin_frame(WorldHandle handle, float alpha) {
    cask::WorldView world(handle);
    int* frames = world.get<int>(frame_count_id);
    int* ticks = world.get<int>(tick_count_id);
    (*frames)++;
    if (*frames % 10000 == 0) {
        std::printf("[FRAME] frames=%d, ticks=%d, alpha=%.2f\n", *frames, *ticks, alpha);
    }
    return handle;
}

WorldHandle plugin_shutdown(WorldHandle handle) {
    std::printf("Shutdown: %d ticks, %d frames\n", tick_count, frame_count);
    tick_count = 0;
    frame_count = 0;
    return handle;
}

static const char* defines_components[] = {"TickCount", "FrameCount"};

static PluginInfo plugin_info = {
    .name = "TimingPlugin",
    .defines_components = defines_components,
    .requires_components = nullptr,
    .defines_count = 2,
    .requires_count = 0,
    .init_fn = plugin_init,
    .tick_fn = plugin_tick,
    .frame_fn = plugin_frame,
    .shutdown_fn = plugin_shutdown
};

extern "C" PluginInfo* get_plugin_info() {
    return &plugin_info;
}
