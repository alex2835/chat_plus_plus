#pragma once

class IController
{
public:
    IController() = default;
    virtual ~IController() = default;
    virtual awaitable<void> call( const size_t sessionId, const json& msg ) = 0;
};