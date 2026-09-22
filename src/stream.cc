#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "stream.h"

#include <spdlog/spdlog.h>

namespace loquat
{

    void Stream::Enqueue(std::vector<Byte> data)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;
        outbuf.push_back(std::move(data));
    }

    void Stream::SetBytesNeeded(std::size_t bytes_needed)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        io_buffer_.bytes_needed_ = bytes_needed;

        if (bytes_needed == 0)
        {
            spdlog::debug("bytes_needed 0");
        }

        // Resize if needed
        auto &inbuf = io_buffer_.read_buffer_;
        if (bytes_needed > inbuf.size())
        {
            inbuf.resize(bytes_needed);
        }
    }

    int Stream::PktsEnqueued(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        return static_cast<int>(outbuf.size());
    }

    void Stream::OnWrite(int sock_fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        while (!outbuf.empty())
        {
            auto &msg = outbuf.front();

            auto buf = msg.data() + io_buffer_.write_queue_head_offset_;
            auto len = msg.size() - io_buffer_.write_queue_head_offset_;

            auto written = ::send(sock_fd, buf, len, 0);

            if (written < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }
                else
                {
                    std::ostringstream errinfo;
                    errinfo << "send:" << strerror(errno);
                    throw std::runtime_error(errinfo.str());
                }
            }
            else if (static_cast<std::size_t>(written) < len)
            {
                io_buffer_.write_queue_head_offset_ += written;
                return;
            }
            else
            {
                io_buffer_.write_queue_head_offset_ = 0;
            }

            outbuf.pop_front();
        }
    }

    void Stream::OnRead(int sock_fd)
    {
        if (type_ == Type::Unframed)
        {
            recvUnframed(sock_fd);
        }
        else
        {
            recvFramed(sock_fd);
        }
    }

    void Stream::recvUnframed(int sock_fd)
    {
        std::vector<Byte> recv_data;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto &inbuf = io_buffer_.read_buffer_;

            auto buf = inbuf.data() + io_buffer_.read_bytes_;
            auto len = inbuf.size();

            auto bytes_in = ::recv(sock_fd, buf, len, 0);
            if (bytes_in <= 0)
            {
                if (bytes_in == 0)
                {
                    return;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }

                std::ostringstream errinfo;
                errinfo << "recv:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }

            io_buffer_.read_bytes_ = bytes_in;
            recv_data.assign(io_buffer_.read_buffer_.begin(),
                             io_buffer_.read_buffer_.begin() + io_buffer_.read_bytes_);
            io_buffer_.read_bytes_ = 0;
        }

        // invoke callback outside the lock
        OnRecv(std::move(recv_data));
    }

    void Stream::recvFramed(int sock_fd)
    {
        std::vector<Byte> recv_data;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto &inbuf = io_buffer_.read_buffer_;
            auto inbuf_start = inbuf.data() + io_buffer_.read_bytes_;

            auto bytes_in = ::recv(sock_fd, inbuf_start, io_buffer_.bytes_needed_, 0);
            if (bytes_in <= 0)
            {
                if (bytes_in == 0)
                {
                    return;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }

                std::ostringstream errinfo;
                errinfo << "recv:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }

            io_buffer_.read_bytes_ += bytes_in;

            if (io_buffer_.bytes_needed_ == io_buffer_.read_bytes_)
            {
                recv_data.assign(io_buffer_.read_buffer_.begin(),
                                 io_buffer_.read_buffer_.begin() + io_buffer_.read_bytes_);
                io_buffer_.read_bytes_ = 0;
            }
        }

        if (!recv_data.empty())
        {
            // invoke callback outside the lock
            OnRecv(std::move(recv_data));
        }
    }
}