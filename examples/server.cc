#include <iostream>

#include "epoll.h"
#include "listener.h"

using namespace loquat;

static Epoll poller;

class ImplConnection : public Connection
{
    public:
        ImplConnection(int listen_fd) : Connection(listen_fd) {};

        void OnRecv(std::vector<Byte>& data) override
        {
            std::cout << "Receive " << data.size() << " bytes:" << std::endl;
            std::cout << data.data() << std::endl;

            // echo
            Enqueue(data);
        }
};

class ImplListener : public Listener
{
    public:
        void OnAccept(int listen_sock) override
        {
            auto connection_ptr = std::make_shared<ImplConnection>(listen_sock);
            poller.Join(connection_ptr->Sock(), connection_ptr);
        }
};

int main( int argc,      // Number of strings in array argv
        char *argv[],   // Array of command-line argument strings
        char *envp[] )  // Array of environment variable strings
{
    auto p_listener = std::make_shared<ImplListener>();

    poller.Join(p_listener->Sock(), p_listener);

    p_listener->Listen("127.0.0.1", 12138);

    poller.Wait();

    return 0;
}