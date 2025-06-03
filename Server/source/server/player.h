#ifndef IMPL_PLAYER_H
#define IMPL_PLAYER_H

#include "network.h"
#include "../protocol/packets.h"

#include "../math/math.h"

typedef enum
{
	hell = -1,
	overworld = 0,
	end = 1
}
world_type_t;

typedef enum
{
	handshake = 0,
	status,
	login,
	play
}
connection_state_t;

class c_player
{
private:
public:
	std::string			name;
	connection_state_t	state;
	uint64_t			last_keep_alive;
	socket_t			client_fd;
	void*				server_ptr;

	vec3d_t position;
	angle_t rotation;
	bool on_ground;

	c_player() : name(""), state(connection_state_t::handshake) { }
	c_player(const c_player&) = delete;
	c_player& operator=(const c_player&) = delete;

	void on_receive(c_packet& packet);
	void send_packet(c_packet& packet);
	void send_message(std::string& message);
};

#endif