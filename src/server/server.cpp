#include "pch.hpp"
#include "server.hpp"

void Server::run()
{
    try
    {
        asio::signal_set signals( ioContext_, SIGINT, SIGTERM );
        signals.async_wait(
            [&]( const boost::system::error_code& ec, int signal_number )
            {
                if ( ec )
                    std::cerr << "Signal handler error: " << ec.message() << "\n";
                std::cout << "\nShutting down server due to signal " << signal_number << "...\n";
                closeAllSessions();
                ioContext_.stop();
            } );

        asio::co_spawn( ioContext_, startListener( address_, port_ ), asio::detached );

        ioContext_.run();

        std::cout << "Server stopped.\n";
    }
    catch ( const std::exception& e )
    {
        std::cerr << "Server error: " << e.what() << "\n";
    }
}

awaitable<void> Server::startListener( std::string_view address, const int port )
{
    try
    {
        auto executor = co_await asio::this_coro::executor;
        const auto endpoint = tcp::endpoint( asio::ip::make_address( address ), port );
        std::cout << "Info: Starting listener on " << address << ":" << port << "\n";

        tcp::acceptor acceptor( executor );
        acceptor.open( endpoint.protocol() );
        acceptor.set_option( asio::socket_base::reuse_address( true ) );
        acceptor.bind( endpoint );
        acceptor.listen( asio::socket_base::max_listen_connections );

        for ( ;; )
        {
            auto socket = co_await acceptor.async_accept( asio::use_awaitable );

            // Create new session
            size_t sessionId = nextSessionId_++;
            auto session = std::make_shared<Session>( *this, sessionId, std::move( socket ) );
            co_await addSession( sessionId, session );

            std::cout << "Info: New session created: " << sessionId << "\n";

            // Start session in its own coroutine
            asio::co_spawn( executor, session->start(), asio::detached );
        }
    }
    catch ( const boost::system::system_error& se )
    {
        std::cerr << "Listener error: " << formatWebSocketError( se.code() ) << "\n";
    }

    std::cout << "Listener stopped.\n";
    co_return;
}

awaitable<void> Server::broadcast( const std::string& message )
{
    const auto sessionsCopy = co_await getSessions();
    co_await sendToSessions( sessionsCopy, message );
}

awaitable<void> Server::broadcastExcept( const size_t excludeId,
                                         const std::string& message )
{
    const auto sessionsCopy = co_await getSessions();
    const auto filterClause = [excludeId]( const auto& session ) { return session->getSessionId() != excludeId; };
    auto filteredSessionsCopy = sessionsCopy | std::views::filter( filterClause );
    co_await sendToSessions( filteredSessionsCopy, message );
}

awaitable<void> Server::sendToSession( const size_t sessionId, const std::string& message )
{
    auto session = co_await findSession( sessionId );
    if ( not session )
        co_return;
        
    auto sessions = { session };
    co_await sendToSessions( sessions, message );
}

awaitable<void> Server::addSession( const size_t sessionId, std::shared_ptr<Session> session )
{
    co_await asio::post( asio::bind_executor( sessionStrand_, asio::use_awaitable ) );

    sessions_.emplace( sessionId, std::move( session ) );
}

void Server::removeSession( size_t sessionId )
{
    asio::post( sessionStrand_,
        [this, sessionId]()
        {
            auto it = sessions_.find( sessionId );
            if ( it != sessions_.end() )
            {
                std::cout << "Info: Removing session " << sessionId << "\n";
                sessions_.erase( it );
            }
        } );
}

awaitable<std::vector<std::shared_ptr<Session>>> Server::getSessions() const
{
    co_await asio::post( asio::bind_executor( sessionStrand_, asio::use_awaitable ) );

    std::vector<std::shared_ptr<Session>> result;
    result.reserve( sessions_.size() );
    for ( const auto& [id, session] : sessions_ )
        result.push_back( session );
    co_return result;
}

awaitable<std::shared_ptr<Session>> Server::findSession( const size_t sessionId ) const
{
    co_await asio::post( asio::bind_executor( sessionStrand_, asio::use_awaitable ) );

    auto it = sessions_.find( sessionId );
    if ( it != sessions_.end() )
        co_return it->second;
    co_return nullptr;
}

void Server::closeAllSessions()
{
    asio::post( sessionStrand_,
        [this]()
        {
            for ( auto& [id, session] : sessions_ )
                session->close();
            sessions_.clear();
        } );
}
