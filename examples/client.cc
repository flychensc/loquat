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
    Epoll poller = Epoll();
    Connector connector = Connector(poller);
    connector.Connect("127.0.0.1", 12138);

    auto msg = std::string("Hello World");
    std::vector<Byte> data(msg.data(), msg.data()+msg.size());

    auto recv_call = [](std::vector<Byte>& data) -> void {
        std::cout << "Receive " << data.size() << " bytes:" << std::endl;
        std::cout << data.data() << std::endl;
        return;
    };
    connector.RegisterOnRecvCallback(recv_call);

    connector.Send(data);

    return 0;
}