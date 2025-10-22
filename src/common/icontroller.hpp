#pragma once

class IController
{
public:
    IController() = default;
    virtual ~IController() = default;
    virtual awaitable<void> call( const json& msg ) = 0;
};