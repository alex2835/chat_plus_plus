#pragma once
#include <chrono>
#include <iomanip>
#include <sstream>

inline std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timePoint = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timePoint), "%H:%M:%S");
    return ss.str();
}