cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "./client/*.cpp")
add_executable(client ${SOURCES})
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(client PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp")
target_link_libraries(client PRIVATE
    boost::boost
    nlohmann_json::nlohmann_json
    magic_enum::magic_enum
    ftxui::ftxui)

file(GLOB_RECURSE SOURCES "./server/*.cpp")
add_executable(server ${SOURCES})
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp")
target_link_libraries(server PRIVATE
    boost::boost
    magic_enum::magic_enum
    nlohmann_json::nlohmann_json)
