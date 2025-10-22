#pragma once
#include "icontroller.hpp"

class MessageDispatcher
{
    std::unordered_map<std::string, std::shared_ptr<IController>> controllers_;

public:
    MessageDispatcher() = default;
    ~MessageDispatcher() = default;

    template <typename EnumType, typename ControllerType>
    void addController( EnumType type, ControllerType&& controller )
    {
        std::string typeName = std::string( magic_enum::enum_name( type ) );
        if ( controllers_.find( typeName ) != controllers_.end() )
            throw std::runtime_error( "IController already exists: " + typeName );
        auto ref = std::make_shared<ControllerType>( std::forward<ControllerType>( controller ) );
        controllers_.emplace( typeName, std::move( ref ) );
    }

    awaitable<void> dispatch( const json& message )
    {
        std::string name = message.at( "metadata" ).at( "type" ).get<std::string>();
        json data = message.at( "data" );

        auto it = controllers_.find( name );
        if ( it != controllers_.end() )
            co_await it->second->call( data );
        else
            std::cerr << "Error: IController not found for type: " << name << "\n";
    }
};