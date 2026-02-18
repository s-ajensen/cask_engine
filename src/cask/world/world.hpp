#pragma once

#include <cask/abi.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class World {
    std::unordered_map<std::string, uint32_t> component_ids_;
    std::vector<void*> components_;
    std::vector<ComponentDeleter> deleters_;

    using IdIterator = std::unordered_map<std::string, uint32_t>::iterator;

    IdIterator find_component(const char* name) {
        return component_ids_.find(std::string(name));
    }

    bool is_missing(IdIterator iterator) {
        return iterator == component_ids_.end();
    }

public:
    uint32_t register_component(const char* name) {
        auto existing = find_component(name);
        if (!is_missing(existing)) {
            return existing->second;
        }
        
        uint32_t new_id = static_cast<uint32_t>(components_.size());
        components_.push_back(nullptr);
        deleters_.push_back(nullptr);
        component_ids_[std::string(name)] = new_id;
        return new_id;
    }

    void bind(uint32_t component_id, void* data) {
        if (components_[component_id] != nullptr) {
            throw std::runtime_error("Component already bound");
        }
        components_[component_id] = data;
    }

    void* get_component(uint32_t component_id) {
        return components_[component_id];
    }

    template<typename T>
    T* get(uint32_t component_id) {
        return static_cast<T*>(get_component(component_id));
    }

    void register_and_bind(const char* name, void* data, ComponentDeleter deleter) {
        uint32_t component_id = register_component(name);
        bind(component_id, data);
        deleters_[component_id] = deleter;
    }

    void* resolve(const char* name) {
        auto found = find_component(name);
        if (is_missing(found)) {
            return nullptr;
        }
        return components_[found->second];
    }

    void destroy(const char* name) {
        auto found = find_component(name);
        if (is_missing(found)) {
            return;
        }
        uint32_t component_id = found->second;
        if (deleters_[component_id] != nullptr) {
            deleters_[component_id](components_[component_id]);
        }
        components_[component_id] = nullptr;
        deleters_[component_id] = nullptr;
        component_ids_.erase(found);
    }
};
