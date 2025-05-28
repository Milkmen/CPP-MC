#include "server/server.h"

#include <thread>

int main() 
{
    c_server server = c_server("config.ini");
    server.run();
    return 0;
}
