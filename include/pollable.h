#pragma once

namespace loquat
{
    class Pollable
    {
    public:
        virtual ~Pollable() = default;
    };

    class Acceptable : virtual public Pollable
    {
    public:
        /** @brief register a callback to handle new connection
         * @param listen_sock listening sock id
         */
        virtual void OnAccept(int listen_sock) = 0;
    };

    class ReadWritable : virtual public Pollable
    {
    protected:
        /** @brief register a callback to handle message in
         * @param sock_fd sock id
         */
        virtual void OnRead(int sock_fd) = 0;
        /** @brief register a callback to handle message out
         * @param sock_fd sock id
         */
        virtual void OnWrite(int sock_fd) = 0;
        friend class Epoll;
    };

    class Closable : virtual public Pollable
    {
    public:
        /** @brief register a callback to handle connection disconnected
         * @param sock_fd sock id
         */
        virtual void OnClose(int sock_fd) = 0;
        friend class Epoll;
    };
}