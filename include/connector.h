#pragma once

#include <string>

#include "stream.h"

namespace loquat
{
    class Connector : public Stream
    {
        public:
            Connector();
            ~Connector();

            Connector( const Connector& ) = delete;
            Connector( Connector&& ) = delete;

            int Sock() { return sock_fd_; };

            void Bind(const std::string& ip4addr, int port);
            void Connect(const std::string& ip4addr, int port);

            void OnRecv(int sock_fd) override;
        private:
            int sock_fd_;
            bool connect_flag_;
    };
}