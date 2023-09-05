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
            Stream() : conn_fd_(-1) {};
            Stream(int fd) : conn_fd_(fd) {};

            void Send(std::vector<Byte>& data);
            void RegisterOnRecvCallback(callback_recv_t callback) { recv_callback_ = callback; };

            void WantRecv();
            void WantRecv(std::size_t bytes_needed);
        protected:
            int conn_fd_;
            void onSend();
            void onRecv();

            friend class Listener;
        private:
            callback_recv_t recv_callback_;
            IOBuffer io_buffer_;

            void recvAdv();
            void recvRaw();
    };
}