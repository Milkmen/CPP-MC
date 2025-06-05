#ifndef IMPL_ENTITY_H
#define IMPL_ENTITY_H

#include "network.h"
#include <stdint.h>

typedef enum
{
	player,
	entity
}
entity_type_t;

typedef struct
{
	entity_type_t type;
	union
	{
		socket_t player_key;
		uint32_t entity_key;
	};
}
entity_entry_t;

#endif