#pragma once
#include <chrono>
#include <iomanip>
#include <sstream>

// Helper function to format WebSocket errors
inline std::string formatWebSocketError( const boost::system::error_code& errorCode )
{
    if ( errorCode == websocket::error::closed )
        return "WebSocket closed normally";
    if ( errorCode == boost::asio::error::connection_reset )
        return "Connection reset by client";
    if ( errorCode == boost::asio::error::connection_aborted )
        return "Connection aborted";
    if ( errorCode == boost::asio::error::timed_out )
        return "Connection timed out";
    
    // Extract just the error message without the long location info
    std::string errorMsg = errorCode.message();
    size_t pos = errorMsg.find( " [" );
    if ( pos != std::string::npos )
        errorMsg = errorMsg.substr( 0, pos );

    return errorMsg;
}

inline std::string getTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto timePoint = std::chrono::system_clock::to_time_t( now );
    std::stringstream ss;
    ss << std::put_time( std::localtime( &timePoint ), "%H:%M:%S" );
    return ss.str();
}