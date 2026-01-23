#pragma once

#include <cask/abi.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include <stdexcept>
#include <algorithm>

namespace cask {
namespace deps {

using DefinerMap = std::unordered_map<std::string, PluginInfo*>;
using DependentsMap = std::unordered_map<PluginInfo*, std::vector<PluginInfo*>>;
using InDegreeMap = std::unordered_map<PluginInfo*, int>;

inline DefinerMap build_definer_map(const std::vector<PluginInfo*>& plugins) {
    DefinerMap component_to_definer;
    for (PluginInfo* plugin : plugins) {
        for (size_t index = 0; index < plugin->defines_count; ++index) {
            std::string component = plugin->defines_components[index];
            if (component_to_definer.find(component) != component_to_definer.end()) {
                throw std::runtime_error("Duplicate definer for component: " + component);
            }
            component_to_definer[component] = plugin;
        }
    }
    return component_to_definer;
}

inline void add_dependency(PluginInfo* definer, PluginInfo* dependent,
                           DependentsMap& dependents, InDegreeMap& in_degree) {
    if (definer != dependent) {
        dependents[definer].push_back(dependent);
        in_degree[dependent]++;
    }
}

inline InDegreeMap initialize_in_degrees(const std::vector<PluginInfo*>& plugins) {
    InDegreeMap in_degree;
    for (PluginInfo* plugin : plugins) {
        in_degree[plugin] = 0;
    }
    return in_degree;
}

inline void build_dependency_edges(const std::vector<PluginInfo*>& plugins,
                                   const DefinerMap& definer_map,
                                   DependentsMap& dependents,
                                   InDegreeMap& in_degree) {
    for (PluginInfo* plugin : plugins) {
        for (size_t index = 0; index < plugin->requires_count; ++index) {
            std::string component = plugin->requires_components[index];
            auto iter = definer_map.find(component);
            if (iter == definer_map.end()) {
                throw std::runtime_error("Missing dependency: " + component);
            }
            add_dependency(iter->second, plugin, dependents, in_degree);
        }
    }
}

inline std::queue<PluginInfo*> find_ready_plugins(const std::vector<PluginInfo*>& plugins,
                                                   const InDegreeMap& in_degree) {
    std::queue<PluginInfo*> ready;
    for (PluginInfo* plugin : plugins) {
        if (in_degree.at(plugin) == 0) {
            ready.push(plugin);
        }
    }
    return ready;
}

inline std::vector<PluginInfo*> topological_sort(const std::vector<PluginInfo*>& plugins,
                                                  DependentsMap& dependents,
                                                  InDegreeMap& in_degree) {
    std::queue<PluginInfo*> ready = find_ready_plugins(plugins, in_degree);
    std::vector<PluginInfo*> result;

    while (!ready.empty()) {
        PluginInfo* current = ready.front();
        ready.pop();
        result.push_back(current);

        for (PluginInfo* dependent : dependents[current]) {
            in_degree[dependent]--;
            if (in_degree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (result.size() != plugins.size()) {
        std::string message = "circular dependency detected involving: ";
        for (PluginInfo* plugin : plugins) {
            if (std::find(result.begin(), result.end(), plugin) == result.end()) {
                message += plugin->name;
                message += " ";
            }
        }
        throw std::runtime_error(message);
    }

    return result;
}

inline std::vector<PluginInfo*> resolve(std::vector<PluginInfo*> plugins) {
    DefinerMap definer_map = build_definer_map(plugins);
    DependentsMap dependents;
    InDegreeMap in_degree = initialize_in_degrees(plugins);
    build_dependency_edges(plugins, definer_map, dependents, in_degree);

    return topological_sort(plugins, dependents, in_degree);
}

}
}
