#pragma once

#include <map>
#include <string>

#include "pollable.h"
#include "stream.h"

namespace loquat
{
    class Connection : public Stream
    {
        public:
            Connection(int listen_fd);
            ~Connection();

            Connection( const Connection& ) = delete;
            Connection( Connection&& ) = delete;

            int Sock() { return sock_fd_; };
        private:
            int sock_fd_;
    };

    class Listener : public Acceptable
    {
        public:
            static const int kMaxConnections = 20;

            Listener(int max_connections);
            Listener() : Listener(kMaxConnections) {};
            ~Listener();

            Listener( const Listener& ) = delete;
            Listener( Listener&& ) = delete;

            int Sock() { return listen_fd_; };

            void Listen(const std::string& ip4addr, int port);
        private:
            int listen_fd_;
            int backlog_;

            void onConnect(int fd);
            void onDisconnect(int fd);
    };
}