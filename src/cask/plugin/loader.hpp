#pragma once

#include <cask/abi.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

struct LoadResult {
    void* handle;
    PluginInfo* info;
};

using GetPluginInfoFn = PluginInfo* (*)();

#if defined(__APPLE__)

#include <dlfcn.h>

inline LoadResult macos_strategy(const char* path) {
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        throw std::runtime_error(std::string("Failed to load plugin: ") + dlerror());
    }
    
    auto get_info = reinterpret_cast<GetPluginInfoFn>(dlsym(handle, "get_plugin_info"));
    if (!get_info) {
        dlclose(handle);
        throw std::runtime_error(std::string("Plugin missing get_plugin_info: ") + path);
    }
    
    PluginInfo* info = get_info();
    return {handle, info};
}

inline void macos_unload(void* handle) {
    dlclose(handle);
}

inline LoadResult native_strategy(const char* path) { return macos_strategy(path); }
inline void native_unload(void* handle) { macos_unload(handle); }

#elif defined(__linux__)

#include <dlfcn.h>

inline LoadResult linux_strategy(const char* path) {
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        throw std::runtime_error(std::string("Failed to load plugin: ") + dlerror());
    }
    
    auto get_info = reinterpret_cast<GetPluginInfoFn>(dlsym(handle, "get_plugin_info"));
    if (!get_info) {
        dlclose(handle);
        throw std::runtime_error(std::string("Plugin missing get_plugin_info: ") + path);
    }
    
    PluginInfo* info = get_info();
    return {handle, info};
}

inline void linux_unload(void* handle) {
    dlclose(handle);
}

inline LoadResult native_strategy(const char* path) { return linux_strategy(path); }
inline void native_unload(void* handle) { linux_unload(handle); }

#elif defined(_WIN32)

#include <windows.h>

inline LoadResult windows_strategy(const char* path) {
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        throw std::runtime_error(std::string("Failed to load plugin: ") + path);
    }
    
    auto get_info = reinterpret_cast<GetPluginInfoFn>(GetProcAddress(handle, "get_plugin_info"));
    if (!get_info) {
        FreeLibrary(handle);
        throw std::runtime_error(std::string("Plugin missing get_plugin_info: ") + path);
    }
    
    PluginInfo* info = get_info();
    return {reinterpret_cast<void*>(handle), info};
}

inline void windows_unload(void* handle) {
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

inline LoadResult native_strategy(const char* path) { return windows_strategy(path); }
inline void native_unload(void* handle) { windows_unload(handle); }

#else
#error "Unsupported platform"
#endif

class Loader {
    std::function<LoadResult(const char*)> load_fn_;

public:
    template<typename F>
    explicit Loader(F load_fn) : load_fn_(load_fn) {}

    LoadResult load(const char* path) {
        return load_fn_(path);
    }

    std::vector<LoadResult> load_all(const std::vector<const char*>& paths) {
        std::vector<LoadResult> result;
        for (const char* path : paths) {
            result.push_back(load(path));
        }
        return result;
    }
};
