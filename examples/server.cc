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
    listener.Listen4("127.0.0.1", 12138);
    return 0;
}