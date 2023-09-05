#pragma once

#include <map>
#include <string>

#include "epoll.h"
#include "stream.h"

namespace loquat
{
    class Listener
    {
        using callback_connect_t = std::function<void(Stream& stream)>;
        using callback_disconnect_t = std::function<void(Stream& stream)>;

        public:
            static const int kMaxConnections = 20;

            Listener(Epoll& poller, int max_connections);
            Listener(Epoll& poller) : Listener(poller, kMaxConnections) {};
            ~Listener();

            Listener( const Listener& ) = delete;
            Listener( Listener&& ) = delete;

            void RegisterOnConnectCallback(callback_connect_t callback) { connect_callback_ = callback; };
            void RegisterOnDisconnectCallback(callback_disconnect_t callback) { disconnect_callback_ = callback; };

            void Listen(const std::string& ip4addr, int port);
        private:
            int listen_fd_;
            int backlog_;
            Epoll& epoll_;
            callback_connect_t connect_callback_;
            callback_disconnect_t disconnect_callback_;
            std::map<int, Stream> connections_;

            void onConnect(int fd);
            void onDisconnect(int fd);
    };
}