#pragma once

#include <cask/abi.h>
#include <cask/deps/graph.hpp>
#include <cask/world/abi_internal.hpp>
#include <vector>

class PluginRegistry {
    std::vector<PluginInfo*> plugins_;
    std::vector<PluginInfo*> init_order_;

public:
    void add(PluginInfo* plugin) {
        plugins_.push_back(plugin);
    }

    void initialize(World& world) {
        init_order_ = cask::deps::resolve(plugins_);
        WorldHandle handle{&world};
        for (PluginInfo* plugin : init_order_) {
            if (plugin->init_fn) {
                plugin->init_fn(&handle);
            }
        }
    }

    void shutdown(World& world) {
        WorldHandle handle{&world};
        for (auto iter = init_order_.rbegin(); iter != init_order_.rend(); ++iter) {
            if ((*iter)->shutdown_fn) {
                (*iter)->shutdown_fn(&handle);
            }
        }
    }
};
