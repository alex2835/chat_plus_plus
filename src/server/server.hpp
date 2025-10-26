#pragma once
#include "common/helpers.hpp"
#include "common/message.hpp"
#include "common/message_dispatcher.hpp"
#include "session.hpp"

class Server
{
private:
    std::string_view address_;
    int port_;
    asio::io_context ioContext_;
    asio::strand<asio::io_context::executor_type> sessionStrand_;
    std::unordered_map<size_t, std::shared_ptr<Session>> sessions_;
    std::atomic<size_t> nextSessionId_{ 0 };
    MessageDispatcher messageDispatcher_;

public:
    Server( std::string_view address,
            int port,
            int threads = 1 )
        : ioContext_( threads ),
          sessionStrand_( asio::make_strand( ioContext_ ) ),
          address_( address ),
          port_( port )
    {}

    asio::io_context& getIOContext()
    {
        return ioContext_;
    }

    void run();
    awaitable<void> startListener( std::string_view address, const int port );

    awaitable<void> broadcast( const std::string& message );
    awaitable<void> broadcastExcept( const size_t excludeId, const std::string& message );
    awaitable<void> sendToSession( const size_t sessionId, const std::string& message );

    awaitable<void> addSessions( const size_t sessionId, std::shared_ptr<Session> session );
    awaitable<std::vector<std::shared_ptr<Session>>> getSessions() const;
    awaitable<std::shared_ptr<Session>> findSession( const size_t sessionId ) const;
    void removeSession( size_t sessionId );
    void closeAllSessions();

    awaitable<void> dispatch( const size_t sessionId, const json& message )
    {
        co_await messageDispatcher_.dispatch( sessionId, message );
    }

    template <typename EnumType, typename ControllerType>
    void addController( EnumType type, ControllerType&& controller )
    {
        messageDispatcher_.addController( type, std::forward<ControllerType>( controller ) );
    }

    template <std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, std::shared_ptr<Session>>
    awaitable<void> sendToSessions( Range&& sessions, const std::string& message )
    {
        for ( auto& session : sessions )
        {
            try
            {
                co_await session->send( message );
            }
            catch ( ... )
            {
                session->close();
                removeSession( session->getSessionId() );
            }
        }
    }
};
