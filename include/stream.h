#pragma once

#include <mutex>
#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    class Stream : public ReadWritable, public Closable
    {
    public:
        enum class StreamType {
            Unframed,    // 不需要长度
            Framed       // 需要长度
        };

        /** @brief enqueue output data
         * @param data output data
         */
        void Enqueue(const std::vector<Byte> &data);

    protected:
        virtual void OnRecv(const std::vector<Byte> &data) = 0;
        /** @brief Read from sock, up to bytes_needed bytes
         * @param bytes_needed update bytes needed
         */
        void SetBytesNeeded(std::size_t bytes_needed);

        void OnClose(int sock_fd) override {};
        void OnWrite(int sock_fd) override;
        void OnRead(int sock_fd) override;

    private:
        void recvUnframed(int sock_fd);
        void recvFramed(int sock_fd);

        IOBuffer io_buffer_;
        std::mutex mutex_;
    };
}
