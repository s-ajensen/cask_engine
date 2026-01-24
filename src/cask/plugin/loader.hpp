#pragma once

#include <cask/abi.h>
#include <functional>
#include <vector>

class Loader {
    std::function<PluginInfo*(const char*)> load_fn_;

public:
    template<typename F>
    explicit Loader(F load_fn) : load_fn_(load_fn) {}

    PluginInfo* load(const char* path) {
        return load_fn_(path);
    }

    std::vector<PluginInfo*> load_all(const std::vector<const char*>& paths) {
        std::vector<PluginInfo*> result;
        for (const char* path : paths) {
            result.push_back(load(path));
        }
        return result;
    }
};
