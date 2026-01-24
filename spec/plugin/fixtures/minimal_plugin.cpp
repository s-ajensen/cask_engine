#include <cask/abi.h>

static PluginInfo plugin_info = {
    .name = "MinimalTestPlugin",
    .defines_components = nullptr,
    .requires_components = nullptr,
    .defines_count = 0,
    .requires_count = 0,
    .init_fn = nullptr,
    .tick_fn = nullptr,
    .frame_fn = nullptr,
    .shutdown_fn = nullptr
};

extern "C" PluginInfo* get_plugin_info() {
    return &plugin_info;
}
