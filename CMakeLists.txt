cmake_minimum_required(VERSION 3.10.0)
project(ChatPlusPlus VERSION 0.1.0)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

find_package(boost REQUIRED COMPONENTS asio beast)
find_package(ftxui REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(magic_enum REQUIRED)

add_subdirectory(src)