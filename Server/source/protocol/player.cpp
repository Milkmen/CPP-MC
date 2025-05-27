#include "player.h"

c_player::c_player()
{
	this->name = "";
	this->state = connection_state_t::login;
}