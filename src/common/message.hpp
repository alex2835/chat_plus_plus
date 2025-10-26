#pragma once
#include <magic_enum.hpp>

// Based on metadata choose controller to handle message
struct Metadata
{
    std::string type;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( Metadata, type )
};

template <typename T>
struct Message
{
    Metadata metadata;
    T data;
};

template <typename E, typename T>
std::string makeMessage( E type, const T& data )
{
    const auto typeName = std::string( magic_enum::enum_name( type ) );
    const auto metadata = Metadata{ .type = typeName };
    const auto message = Message<T>{ metadata, data };
    return json( message ).dump();
}

// Specialization for JSON serialization of template class
template <typename T>
struct nlohmann::adl_serializer<Message<T>>
{
    static void to_json( json& j, const Message<T>& msg )
    {
        j = json{ { "metadata", msg.metadata }, { "data", msg.data } };
    }

    static void from_json( const json& j, Message<T>& msg )
    {
        j.at( "metadata" ).get_to( msg.metadata );
        j.at( "data" ).get_to( msg.data );
    }
};