#pragma once
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include "client.hpp"
using namespace ftxui;

class ChatClientUI
{
public:
    ChatClientUI(ChatClient& client) 
        : client_(client),
          screen_(ScreenInteractive::Fullscreen())
    {
        setup_components();
    }

    void run()
    {
        screen_.Loop(main_renderer_);
    }

private:
    void on_connect()
    {
        if (!client_.is_connected()) {
            client_.set_username(username_input_);
            client_.set_server(server_input_, port_input_);
            client_.connect();
        }
    }

    void on_disconnect()
    {
        if (client_.is_connected()) {
            client_.disconnect();
        }
    }

    void on_send()
    {
        if (!message_input_.empty() && client_.is_connected()) {
            client_.send_message(message_input_);
            message_input_.clear();
        }
    }

    bool handle_event(Event event)
    {
        // Tab to cycle through tabs
        if (event == Event::Tab) {
            selected_tab_ = (selected_tab_ + 1) % 2;
            return true;
        }
        return false;
    }

    void setup_components()
    {
        // Connections settings
        username_field_ = Input(&username_input_, "Enter username...");
        server_field_ = Input(&server_input_, "Server address...");
        port_field_ = Input(&port_input_, "Port...");
        connect_button_ = Button("Connect", [this] { on_connect(); });
        disconnect_button_ = Button("Disconnect", [this] { on_disconnect(); });
        
        // Message input and send button
        message_field_ = Input(&message_input_, "Type a message...");
        send_button_ = Button("Send", [this] { on_send(); });
        send_button_ |= CatchEvent([&](Event event) {
            return event == Event::Return ? true : false;
        });
        
        // Tab selector
        tab_toggle_ = Toggle(&tab_entries_, &selected_tab_);
        
        // Build component tree
        auto settings_tab = Container::Vertical({
            username_field_,
            server_field_,
            port_field_,
            Container::Horizontal({
                connect_button_,
                disconnect_button_,
            }),
        });
        
        auto chat_tab = Container::Horizontal({
            message_field_,
            send_button_
        });
        
        
        auto main_container = Container::Vertical({
            tab_toggle_,
            Container::Tab({
                settings_tab,
                chat_tab,
            }, &selected_tab_),
        });
        
        // Add keyboard shortcuts
        auto component_with_events = main_container | CatchEvent([this](Event event) {
            return handle_event(event);
        });
        
        // Create renderer
        main_renderer_ = Renderer(component_with_events, [this] {
            return render();
        });
    }

    Element render()
    {
        auto status_bar = render_status_bar();
        
        Element document;
        if (selected_tab_ == 0) {
            document = vbox({
                status_bar,
                render_settings_view(),
                separator(),
                render_tab_info(),
            });
        } else {
            document = vbox({
                status_bar,
                render_chat_view(),
                render_message_input_display(),
                separator(),
                render_tab_info(),
            });
        }
        return document;
    }

    Element render_status_bar()
    {
        return hbox({
            text("Status: "),
            text(client_.get_status()) | 
                (client_.is_connected() ? color(Color::Green) : color(Color::Red)),
            filler(),
            text("User: " + username_input_),
        }) | borderStyled(ROUNDED);
    }

    Element render_settings_view()
    {
        return vbox({
            text("Connection Settings") | bold | center,
            separator(),
            hbox({text("Username: ")}),
            username_field_->Render() | borderStyled(ROUNDED),
            separator(),
            hbox({text("Server: ")}),
            server_field_->Render() | borderStyled(ROUNDED),
            hbox({text("Port: ")}),
            port_field_->Render() | borderStyled(ROUNDED),
            separator(),
            hbox({
                connect_button_->Render() | 
                    (client_.is_connected() ? dim : color(Color::Green)),
                text("  "),
                disconnect_button_->Render() | 
                    (!client_.is_connected() ? dim : color(Color::Red)),
            }) | center,
        }) | borderStyled(ROUNDED) | yflex;
    }

    Element render_chat_view()
    {
        auto msgs = client_.get_messages();
        Elements message_elements;
        
        for (const auto& msg : msgs) {
            if (msg.find("[System]") != std::string::npos) {
                message_elements.push_back(text(msg) | color(Color::Yellow));
            } else if (msg.find("[Error]") != std::string::npos) {
                message_elements.push_back(text(msg) | color(Color::Red));
            } else if (msg.find(username_input_ + ":") != std::string::npos) {
                message_elements.push_back(text(msg) | color(Color::Cyan));
            } else {
                message_elements.push_back(text(msg));
            }
        }
        
        return vbox({
            text("Chat Messages") | bold | center,
            separator(),
            vbox(message_elements) | frame | yflex | focusPositionRelative(1.0f, 0.0f),
        }) | borderStyled(ROUNDED) | flex;
    }

    Element render_message_input_display()
    {
        return hbox({
            // text("Message: "),
            // text(message_input_.empty() ? "[Type your message above]" : message_input_),
            message_field_->Render(),
            send_button_->Render() | (message_input_.empty() ? dim : color(Color::Green)),
        }) | borderStyled(ROUNDED);
    }

    Element render_tab_info()
    {
        return hbox({
            text("Active Tab: [") | dim,
            text("1:Chat") | (selected_tab_ == 0 ? color(Color::Cyan) | bold : dim),
            text("] [") | dim,
            text("2:Settings") | (selected_tab_ == 1 ? color(Color::Cyan) | bold : dim),
            text("]  ") | dim,
        });
    }

private:
    enum Tab{ TAB_SETTINGS, TAB_CHAT };

    // Client reference
    ChatClient& client_;
    
    // Screen
    ScreenInteractive screen_;
    
    // UI state
    std::string username_input_;
    std::string message_input_;
    std::string server_input_ = "localhost";
    std::string port_input_ = "8080";
    int selected_tab_ = TAB_SETTINGS;
    std::vector<std::string> tab_entries_ = {"Settings", "Chat"};
    
    // Components
    Component username_field_;
    Component server_field_;
    Component port_field_;
    Component connect_button_;
    Component disconnect_button_;
    
    Component message_field_;
    Component send_button_;
    
    Component tab_toggle_;
    Component main_renderer_;
};


