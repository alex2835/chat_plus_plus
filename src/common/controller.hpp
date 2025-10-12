#pragma once
#include "pch.hpp"
#include <functional>

class Controller {
    std::function<json(const json&)> method_;
    
public:
    Controller() = default;
    virtual ~Controller() = default;
    virtual json call(const json& request) = 0;
};