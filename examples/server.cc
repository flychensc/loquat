#include <iostream>

#include "epoll.h"
#include "listener.h"

using namespace loquat;

static Epoll poller;

class EchoConnection : public Connection
{
    public:
        EchoConnection(int listen_fd) : Connection(listen_fd) {};

        void OnRecv(std::vector<Byte>& data) override
        {
            std::cout << "Receive " << data.size() << " bytes: ";
            std::cout << data.data() << std::endl;

            // echo
            std::cout << "Echo back..." << std::endl << std::endl;
            Enqueue(data);
        }
};

class EchoListener : public Listener
{
    public:
        static const int kMaxConnections = 20;

        EchoListener() : Listener(kMaxConnections) {};

        void OnAccept(int listen_sock) override
        {
            auto connection_ptr = std::make_shared<EchoConnection>(listen_sock);
            std::cout << "Accept " << connection_ptr->Sock() << " from " << listen_sock << std::endl;
            poller.Join(connection_ptr->Sock(), connection_ptr);
        }
};

int main( int argc,      // Number of strings in array argv
        char *argv[],   // Array of command-line argument strings
        char *envp[] )  // Array of environment variable strings
{
    auto p_listener = std::make_shared<EchoListener>();

    poller.Join(p_listener->Sock(), p_listener);

    p_listener->Listen("127.0.0.1", 8000);

    poller.Wait();

    return 0;
}