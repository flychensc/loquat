#include <iostream>
#include <string>
#include <vector>

#include "epoll.h"
#include "connector.h"

using namespace loquat;

int main( int argc,      // Number of strings in array argv
          char *argv[],   // Array of command-line argument strings
          char *envp[] )  // Array of environment variable strings
{
    Epoll poller;
    Connector connector(poller);

    auto msg = std::string("Hello World");
    std::vector<Byte> data(msg.data(), msg.data()+msg.size());

    auto recv_call = [&connector, &poller](std::vector<Byte>& data) -> void {
        std::cout << "Receive " << data.size() << " bytes:" << std::endl;
        std::cout << data.data() << std::endl;

        poller.Terminate();
        return;
    };
    connector.RegisterOnRecvCallback(recv_call);

    connector.Enqueue(data);
    connector.Connect("127.0.0.1", 12138);

    return 0;
}