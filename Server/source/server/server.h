#ifndef IMPL_SERVER_H
#define IMPL_SERVER_H

#include <stdint.h>
#include <map>

#include "network.h"

#include "../protocol/packet.h"
#include "player.h"

typedef struct
{
	uint8_t max_players;
	uint16_t port;
}
server_config_t;

class c_server
{
private:
public:
	server_config_t config;
	std::map<socket_t, c_player> players;
	std::vector<std::string> chat_messages;
	c_server(const char* config_name);
	int run();
	void broadcast(std::string& message);
};

#endif