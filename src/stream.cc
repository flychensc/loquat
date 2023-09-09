#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    using namespace std;

    void Stream::Enqueue(const vector<Byte>& data)
    {
        auto& outbuf = io_buffer_.write_queue_;
        outbuf.push_back(data);
    }

    void Stream::OnWrite(int sock_fd)
    {
        auto& outbuf = io_buffer_.write_queue_;

        while(!outbuf.empty())
        {
            auto& msg = outbuf.front();

            auto buf = msg.data()+io_buffer_.write_queue_head_offset_;
            auto len = msg.size()-io_buffer_.write_queue_head_offset_;

            auto written = ::send(sock_fd, buf, len, 0);

            if (written < 0)
            {
                if (errno == EAGAIN)
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
        auto& inbuf = io_buffer_.read_buffer_;

        auto buf = inbuf.data();
        auto len = inbuf.size();

        auto bytes_in = ::recv(sock_fd, buf, len, 0);
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

        io_buffer_.read_bytes_ = bytes_in;

        vector<Byte> recv_data;
        recv_data.assign(io_buffer_.read_buffer_.begin(), io_buffer_.read_buffer_.begin()+io_buffer_.read_bytes_);

        io_buffer_.read_bytes_ = 0;

        // invoke callback
        OnRecv(recv_data);
    }
}