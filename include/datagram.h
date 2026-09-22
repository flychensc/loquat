#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    class Datagram : public ReadWritable
    {
    public:
        void Enqueue(const SockAddr &toaddr, std::vector<Byte> data);

    protected:
        /** @brief Total pkts queued
         * @return Total pkts queued
         */
        int PktsEnqueued(void);

        /** @brief 用户需要 override 来接收数据报
         *  @param fromaddr 来源地址
         *  @param data 收到的数据 (按值传递，可 move)
         */
        virtual void OnRecv(const SockAddr &fromaddr, std::vector<Byte> data) = 0;

        void OnRead(int sock_fd) override;
        void OnWrite(int sock_fd) override;

    private:
        IOBuffer2 io_buffer_;
        std::mutex mutex_;
    };
}