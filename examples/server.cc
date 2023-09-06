#include <iostream>

#include "epoll.h"
#include "listener.h"

using namespace loquat;

int main( int argc,      // Number of strings in array argv
        char *argv[],   // Array of command-line argument strings
        char *envp[] )  // Array of environment variable strings
{
    Epoll poller = Epoll();
    Listener listener = Listener(poller);

    auto connect_call = [&listener](Stream& stream) -> void {

        stream.RegisterOnRecvCallback([&stream](std::vector<Byte>& data) -> void {
            std::cout << "Receive " << data.size() << " bytes:" << std::endl;
            std::cout << data.data() << std::endl;

            // echo
            stream.Enqueue(data);
        });
    };

    listener.RegisterOnConnectCallback(connect_call);
    listener.Listen("127.0.0.1", 12138);
    return 0;
}