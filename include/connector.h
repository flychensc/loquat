#pragma once

#include <string>

#include "epoll.h"

namespace loquat
{
    class Connector
    {
        public:
            Connector(Epoll& poller);
            ~Connector();

            Connector( const Connector& ) = delete;
            Connector( Connector&& ) = delete;

            void Bind4(const std::string& ip4addr, int port);
            void Connect4(const std::string& ip4addr, int port);
        private:
            int conn_fd_;
            Epoll& epoll_;
    };
}