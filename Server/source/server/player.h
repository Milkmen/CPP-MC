#ifndef IMPL_PLAYER_H
#define IMPL_PLAYER_H

#include "network.h"
#include "../protocol/packets.h"

typedef enum
{
	hell = -1,
	overworld = 0,
	end = 1
}
world_type_t;

typedef enum
{
	invalid = -1,
	handshake,
	login,
	status,
	play
}
connection_state_t;

class c_player
{
private:
public:
	std::string name;
	connection_state_t state;
	socket_t	client_fd;
	void* server_ptr;

	c_player() : name(""), state(connection_state_t::invalid) {}
	void on_receive(c_packet& packet);
	void send_packet(c_packet& packet);
	void send_message(std::string& message);
};

#endif