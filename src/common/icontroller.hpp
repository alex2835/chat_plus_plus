#pragma once
#include "pch.hpp"
#include <functional>

class IController {
public:
    IController() = default;
    virtual ~IController() = default;
    virtual void call(const json& msg) = 0;
};