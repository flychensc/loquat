#pragma once

#include <string>

#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    class Connector : public Stream
    {
    public:
        Connector(Stream::Type type, int domain);
        Connector() : Connector(Stream::Type::Unframed, AF_INET) {}
        Connector(int domain) : Connector(Stream::Type::Unframed, domain) {}
        ~Connector();

        Connector(const Connector &) = delete;
        Connector(Connector &&) = delete;

        /** @brief get sock id
         * @return sock id
         */
        int Sock() { return sock_fd_; };

        /** @brief binding a ipv4/ipv6 address+port
         * @param ipaddr ip address
         * @param port port number
         */
        void Bind(const std::string &ipaddr, int port);
        /** @brief binding a unix path
         * @param unix_path unix path
         */
        void Bind(const std::string &unix_path);

        /** @brief connect a ipv4/ipv6 address+port
         * @param ipaddr ip address
         * @param port port number
         */
        void Connect(const std::string &ipaddr, int port);
        /** @brief connect a unix path
         * @param unix_path unix path
         */
        void Connect(const std::string &unix_path);

        void OnRead(int sock_fd) override;

    private:
        int domain_;
        int sock_fd_;
        bool connect_flag_;
    };
}