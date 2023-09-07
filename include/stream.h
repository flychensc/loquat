#pragma once

#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    class Stream : public Streamable
    {
        public:
            void Enqueue(std::vector<Byte>& data);
            virtual void OnRecv(std::vector<Byte>& data) = 0;

            void OnClose(int sock_fd) override {};
            void OnSend(int sock_fd) override;
            void OnRecv(int sock_fd) override;
        private:
            IOBuffer io_buffer_;
    };
}