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
        enum class Type
        {
            Unframed,
            Framed,
        };

        Stream() : Stream(Type::Unframed) {}
        Stream(Type type) : type_(type) {}

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

        /** @brief Total pkts queued
         * @return Total pkts queued
         */
        int PktsEnqueued(void);

        void OnClose(int sock_fd) override {};
        void OnWrite(int sock_fd) override;
        void OnRead(int sock_fd) override;

    private:
        void recvUnframed(int sock_fd);
        void recvFramed(int sock_fd);

        Type type_;
        IOBuffer io_buffer_;
        std::mutex mutex_;
    };
}
