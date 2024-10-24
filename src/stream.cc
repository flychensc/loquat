#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    using namespace std;

    void Stream::Enqueue(const vector<Byte> &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;
        outbuf.push_back(data);
    }

    void Stream::SetBytesNeeded(std::size_t bytes_needed)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        io_buffer_.bytes_needed_ = bytes_needed;

        // Resize if needed
        auto &inbuf = io_buffer_.read_buffer_;
        if (bytes_needed > inbuf.size())
        {
            inbuf.resize(bytes_needed);
        }
    }

    void Stream::OnWrite(int sock_fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &outbuf = io_buffer_.write_queue_;

        while (!outbuf.empty())
        {
            auto msg = outbuf.front();

            auto buf = msg.data() + io_buffer_.write_queue_head_offset_;
            auto len = msg.size() - io_buffer_.write_queue_head_offset_;

            auto written = ::send(sock_fd, buf, len, 0);

            if (written < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    written = 0;
                    return;
                }
                else
                {
                    stringstream errinfo;
                    errinfo << "send:" << strerror(errno);
                    throw runtime_error(errinfo.str());
                }
            }
            else if (written < len)
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
        vector<Byte> recv_data;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto &inbuf = io_buffer_.read_buffer_;

            auto inbuf_start = inbuf.data() + io_buffer_.read_bytes_;

            auto bytes_in = ::recv(sock_fd, inbuf_start, io_buffer_.bytes_needed_, 0);
            if (bytes_in <= 0)
            {
                if (bytes_in == 0)
                {
                    /* Socket is closed */
                    return;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return;
                }

                stringstream errinfo;
                errinfo << "recv:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }

            io_buffer_.read_bytes_ += bytes_in;

            if (io_buffer_.bytes_needed_ == io_buffer_.read_bytes_)
            {
                recv_data.assign(io_buffer_.read_buffer_.begin(), io_buffer_.read_buffer_.begin() + io_buffer_.read_bytes_);

                io_buffer_.read_bytes_ = 0;
                io_buffer_.bytes_needed_ = 0;
            }
        }

        if (recv_data.size() > 0)
        {
            // invoke callback
            OnRecv(recv_data);
        }
    }
}