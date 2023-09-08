#pragma once

#include <string>

#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    class Connector : public Stream
    {
        public:
            Connector(int domain);
            Connector() : Connector(AF_INET) {};
            ~Connector();

            Connector( const Connector& ) = delete;
            Connector( Connector&& ) = delete;

            int Sock() { return sock_fd_; };

            void Bind(const std::string& ipaddr, int port);
            void Bind(const std::string& unix_path);

            void Connect(const std::string& ipaddr, int port);
            void Connect(const std::string& unix_path);

            void OnRecv(int sock_fd) override;
        private:
            int domain_;
            int sock_fd_;
            bool connect_flag_;
    };
}