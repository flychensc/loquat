#pragma once

#include <mutex>
#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    /** @brief Stream 抽象基类，提供 TCP 收发能力
     *  @note 非线程安全，所有读写必须在同一个 epoll 事件线程中调用
     */
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
         *  @param data output data (按值传递，支持 move)
         */
        void Enqueue(std::vector<Byte> data);

    protected:
        /** @brief 用户需要 override 来接收数据
         *  @param data 收到的数据 (按值传递，可 move)
         */
        virtual void OnRecv(std::vector<Byte> data) = 0;

        /** @brief Read from sock, up to bytes_needed bytes
         *  @param bytes_needed update bytes needed
         */
        void SetBytesNeeded(std::size_t bytes_needed);

        /** @brief Total pkts queued
         *  @return Total pkts queued
         */
        int PktsEnqueued(void);

        void OnClose(int sock_fd) override { (void)sock_fd; };
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