#pragma once
#include <vector>
#include <string>
#include <map>

class Controller {
public:
    Controller() = default;
    ~Controller() = default;

private:
    std::function<void()> method;
};


class RequestResponseHandler {
public:
    RequestResponseHandler() = default;
    ~RequestResponseHandler() = default;
    
    void addController(const std::string& name, Controller&& controller) {
        controllers[name] = std::move( controller );
    }
private:
    std::map<std::string, Controller> controllers;
};