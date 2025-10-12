#pragma once
#include "pch.hpp"
#include "controller.hpp"

class ControllerMapper {
    std::unordered_map<std::string, std::shared_ptr<Controller>> controllers_;

public:
    ControllerMapper() = default;
    ~ControllerMapper() = default;
    
    template <typename E, typename C>
    void addController(E type, C&& controller) {
        std::string typeName = std::string(magic_enum::enum_name(type));
        if (controllers_.find(typeName) != controllers_.end()) {
            throw std::runtime_error("Controller already exists: " + typeName);
        }
        controllers_.emplace(typeName, std::make_shared<Controller>(std::move(controller)));
    }

    json resolveRequest(const std::string& name, const json& message) {
        auto it = controllers_.find(name);
        if (it != controllers_.end()) {
            it->second.call(message);
        } else {
            throw std::runtime_error("Controller not found: " + name);
        }
    }
};