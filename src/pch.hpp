#pragma once

// Boost.Beast
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

// Boost.Asio
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/awaitable.hpp>

// Boost.Program_Options
#include <boost/program_options.hpp>
namespace po = boost::program_options;

// Common namespaces
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using asio::awaitable;

// json
#include <nlohmann/json.hpp>
using nlohmann::json;

// magic_enum
#include <magic_enum.hpp>

// Standard library
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <ranges>
#include <concepts>
#include <algorithm>