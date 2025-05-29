#include "server/server.h"

int main() 
{
    c_server server = c_server("config.ini");
    server.run();
    return 0;
}
