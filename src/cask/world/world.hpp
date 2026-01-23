#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class World {
    std::unordered_map<std::string, uint32_t> component_ids_;
    std::vector<void*> components_;

public:
    uint32_t register_component(const char* name) {
        std::string component_name(name);
        
        auto existing = component_ids_.find(component_name);
        if (existing != component_ids_.end()) {
            return existing->second;
        }
        
        uint32_t new_id = static_cast<uint32_t>(components_.size());
        components_.push_back(nullptr);
        component_ids_[component_name] = new_id;
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
};
