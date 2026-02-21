#pragma once

#include <cask/abi.h>
#include <cask/deps/graph.hpp>
#include <cask/world/abi_internal.hpp>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

class PluginRegistry {
    std::vector<PluginInfo*> plugins_;
    std::vector<PluginInfo*> init_order_;
    std::unordered_set<std::string> initialized_names_;

public:
    void add(PluginInfo* plugin) {
        for (PluginInfo* existing : plugins_) {
            if (std::strcmp(existing->name, plugin->name) == 0) {
                return;
            }
        }
        plugins_.push_back(plugin);
    }

    std::vector<PluginInfo*> initialize(World& world) {
        init_order_ = cask::deps::resolve(plugins_);
        std::vector<PluginInfo*> newly_initialized;
        WorldHandle handle = handle_from_world(&world);
        for (PluginInfo* plugin : init_order_) {
            if (initialized_names_.count(plugin->name)) {
                continue;
            }
            if (plugin->init_fn) {
                plugin->init_fn(handle);
            }
            initialized_names_.insert(plugin->name);
            newly_initialized.push_back(plugin);
        }
        return newly_initialized;
    }

    void shutdown(World& world) {
        WorldHandle handle = handle_from_world(&world);
        for (auto iter = init_order_.rbegin(); iter != init_order_.rend(); ++iter) {
            PluginInfo* plugin = *iter;
            if (plugin->shutdown_fn) {
                plugin->shutdown_fn(handle);
            }
            for (size_t index = 0; index < plugin->defines_count; ++index) {
                world.destroy(plugin->defines_components[index]);
            }
        }
    }

    const std::vector<PluginInfo*>& plugins() const {
        return init_order_;
    }
};
