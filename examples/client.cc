#include <iostream>
#include <string>
#include <vector>

#include "epoll.h"
#include "connector.h"

using namespace loquat;

static Epoll poller;

class ImplConnector : public Connector
{
    public:
        void OnRecv(std::vector<Byte>& data) override
        {
            std::cout << "Receive " << data.size() << " bytes:" << std::endl;
            std::cout << data.data() << std::endl;

            poller.Terminate();
        }
};

int main( int argc,      // Number of strings in array argv
          char *argv[],   // Array of command-line argument strings
          char *envp[] )  // Array of environment variable strings
{
    auto p_connector = std::make_shared<ImplConnector>();

    poller.Join(p_connector->Sock(), p_connector);

    auto msg = std::string("Hello World");
    std::vector<Byte> data(msg.data(), msg.data()+msg.size());
    p_connector->Enqueue(data);

    p_connector->Connect("127.0.0.1", 12138);

    poller.Wait();

    poller.Leave(p_connector->Sock());

    return 0;
}