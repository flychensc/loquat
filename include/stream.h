#pragma once

#include <functional>
#include <vector>

#include "io_buffer.h"

namespace loquat
{
    class Stream
    {
        using callback_recv_t = std::function<void(std::vector<Byte>& data)>;

        public:
            Stream(int fd) : sock_fd_(fd) {};

            void Send(std::vector<Byte>& data);
            void RegisterOnRecvCallback(callback_recv_t callback) { recv_callback_ = callback; };

        protected:
            Stream() : Stream(-1) {};
            void Sock(int fd) { sock_fd_ = fd; };
        private:
            int sock_fd_;
            callback_recv_t recv_callback_;

            void onRecv();
            IOBuffer io_buffer_;
    };
}