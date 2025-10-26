#pragma once
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "client.hpp"
#include "client_data.hpp"
using namespace ftxui;

class ChatClientUI
{
    // Client reference
    ClientData& clientData_;
    ChatClient& client_;

    // Screen
    ScreenInteractive screen_;

    // UI stSate
    enum Tab
    {
        TabSettings,
        TabChat
    };
    int selectedTab_ = TabSettings;
    std::vector<std::string> tabEntries_ = { "Settings", "Chat" };
    std::string usernameInput_;
    std::string addresInput_ = "localhost";
    std::string portInput_ = "8080";
    
    // Rooms
    std::string selectedRoom_;
    int selectedRoomIndex_ = 0;
    std::vector<std::string> roomsRadio_;
    std::string newRoomInput_;
    
    // Message input
    std::string messageInput_;

    // Components
    Component usernameField_;
    Component serverField_;
    Component portField_;
    Component connectButton_;
    Component disconnectButton_;

    Component messageField_;
    Component sendButton_;
    Component roomsRadiobox_;
    Component newRoomField_;
    Component createRoomButton_;

    Component tabToggle_;
    Component mainRenderer_;

public:
    ChatClientUI(  ClientData& database, ChatClient& client )
        : clientData_( database ), 
          client_( client ), 
          screen_( ScreenInteractive::Fullscreen() )
    {
        setupComponents();
    }

    void run()
    {
        screen_.Loop( mainRenderer_ );
    }

private:
    void onConnect()
    {
        if ( !client_.isConnected() )
        {
            clientData_.setUserName( usernameInput_ );
            client_.setServer( addresInput_, portInput_ );
            client_.connect();
        }
    }

    void onDisconnect()
    {
        if ( client_.isConnected() )
            client_.disconnect();
    }

    void onSend()
    {
        if ( !messageInput_.empty() && client_.isConnected() )
        {
            client_.sendChatMessage( selectedRoom_, messageInput_ );
            messageInput_.clear();
        }
    }

    void onCreateRoom()
    {
        if ( !newRoomInput_.empty() && client_.isConnected() )
        {
            client_.sendChatRoom( newRoomInput_ );
            newRoomInput_.clear();
        }
    }

    bool handleEvent( Event event )
    {
        // Tab to cycle through tabs
        if ( event == Event::Tab )
        {
            selectedTab_ = static_cast<Tab>( ( selectedTab_ + 1 ) % 2 );
            return true;
        }
        return false;
    }

    void setupComponents()
    {
        // Connections settings
        usernameField_ = Input( &usernameInput_, "Enter username..." );
        serverField_ = Input( &addresInput_, "Server address..." );
        portField_ = Input( &portInput_, "Port..." );
        connectButton_ = Button( "Connect", [this] { onConnect(); } );
        disconnectButton_ = Button( "Disconnect", [this] { onDisconnect(); } );

        // Message input and send button
        messageField_ = Input( &messageInput_, "Type a message..." );
        sendButton_ = Button( "Send", [this] { onSend(); } );
        sendButton_ |=
            CatchEvent( [&]( Event event ) { return event == Event::Return ? true : false; } );

        // Room selector
        roomsRadiobox_ = Radiobox( &roomsRadio_, &selectedRoomIndex_ );

        // New room creation
        newRoomField_ = Input( &newRoomInput_, "New room name..." );
        createRoomButton_ = Button( "Create Room", [this] { onCreateRoom(); } );

        // Tab selector
        tabToggle_ = Toggle( &tabEntries_, &selectedTab_ );

        // Build component tree
        auto settingsTab = Container::Vertical( {
            usernameField_,
            serverField_,
            portField_,
            Container::Horizontal( {
                connectButton_,
                disconnectButton_,
            } ),
        } );

        auto messageInputContainer = Container::Horizontal( { messageField_, sendButton_ } );
        auto roomsContainer = Container::Vertical( {
            roomsRadiobox_,
            newRoomField_,
            createRoomButton_,
        } );
        auto chatTab = Container::Horizontal( {
            roomsContainer,
            Container::Vertical( { messageInputContainer } ),
        } );

        auto mainContainer = Container::Vertical( {
            tabToggle_,
            Container::Tab(
                {
                    settingsTab,
                    chatTab,
                },
                &selectedTab_ ),
        } );

        // Add keyboard shortcuts
        auto componentWithEvents =
            mainContainer | CatchEvent( [this]( Event event ) { return handleEvent( event ); } );

        // Create renderer
        mainRenderer_ = Renderer( componentWithEvents, [this] { return render(); } );
    }

    Element render()
    {
        auto statusBar = renderStatusBar();

        Element document;
        if ( selectedTab_ == TabSettings )
        {
            document = vbox( {
                statusBar,
                renderSettingsView(),
                separator(),
                renderTabInfo(),
            } );
        }
        else
        {
            document = vbox( {
                statusBar,
                hbox( {
                    renderChatRooms(),
                    vbox( {
                        renderChatRoomMessages(),
                        renderMessageInputDisplay(),
                    } ) | flex,
                } ) | yflex,
                separator(),
                renderTabInfo(),
            } );
        }
        return document;
    }

    Element renderStatusBar()
    {
        const auto statuTextColor = client_.isConnected() 
                                        ? color( Color::Green ) 
                                        : color( Color::Red );
        return hbox( {
                   text( "Status: " ),
                   text( client_.getStatus() ) | statuTextColor,
                   filler(),
                   text( "User: " + usernameInput_ ),
               } ) |
               borderStyled( ROUNDED );
    }

    Element renderSettingsView()
    {
        return vbox( {
                   text( "Connection Settings" ) | bold | center,
                   separator(),
                   hbox( { text( "Username: " ) } ),
                   usernameField_->Render() | borderStyled( ROUNDED ),
                   separator(),
                   hbox( { text( "Server: " ) } ),
                   serverField_->Render() | borderStyled( ROUNDED ),
                   hbox( { text( "Port: " ) } ),
                   portField_->Render() | borderStyled( ROUNDED ),
                   separator(),
                   hbox( {
                       connectButton_->Render() |
                           ( client_.isConnected() ? dim : color( Color::Green ) ),
                       text( "  " ),
                       disconnectButton_->Render() |
                           ( !client_.isConnected() ? dim : color( Color::Red ) ),
                   } ) |
                       center,
               } ) |
               borderStyled( ROUNDED ) | yflex;
    }

    Element renderChatRooms()
    {
        // Update selectedRoom_ based on current selection
        if ( selectedRoomIndex_ >= 0 && selectedRoomIndex_ < roomsRadio_.size() )
            selectedRoom_ = roomsRadio_[selectedRoomIndex_];

        return vbox( {
                   text( "Chat Rooms" ) | bold | center,
                   separator(),
                   roomsRadiobox_->Render() | vscroll_indicator | frame | yflex,
                   separator(),
                   text( "Create New Room" ) | bold,
                   newRoomField_->Render() | borderStyled( ROUNDED ),
                   createRoomButton_->Render() |
                       ( newRoomInput_.empty() || !client_.isConnected() ? dim : color( Color::Green ) ),
               } ) |
               borderStyled( ROUNDED ) | size( WIDTH, GREATER_THAN, 20 );
    }

    Element renderChatRoomMessages()
    {
        const auto msgs = clientData_.getRoomMessages( selectedRoom_ );
        Elements messageElements;
        for ( const auto& msg : msgs )
            messageElements.push_back( text( msg.content ) );
 
        return vbox( {
                        text( "Chat Messages" ) | bold | center,
                        separator(),
                        vbox( messageElements ) | frame | yflex | focusPositionRelative( 1.0f, 0.0f ),
                     } ) | borderStyled( ROUNDED ) | flex;
    }

    Element renderMessageInputDisplay()
    {
        return hbox( {
                   messageField_->Render(),
                   sendButton_->Render() | ( messageInput_.empty() ? dim : color( Color::Green ) ),
               } ) |
               borderStyled( ROUNDED );
    }

    Element renderTabInfo()
    {
        return hbox( {
            text( "Active Tab: [" ) | dim,
            text( "1:Settings" ) |
                ( selectedTab_ == TabSettings ? color( Color::Cyan ) | bold : dim ),
            text( "] [" ) | dim,
            text( "2:Chat" ) | ( selectedTab_ == TabChat ? color( Color::Cyan ) | bold : dim ),
            text( "]  " ) | dim,
        } );
    }
};