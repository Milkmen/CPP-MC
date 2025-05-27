#ifndef MC_PACKETS_H
#define MC_PACKETS_H

#include "packet.h"

class c_packet_c2s 
{
public:
	virtual void deserialize(c_packet& packet) = 0;
	virtual ~c_packet_c2s() = default;
};


class c_packet_s2c 
{
public:
	virtual void serialize(c_packet& packet) const = 0;
	virtual ~c_packet_s2c() = default;
};


/*
    Client to Server Packets
*/

class c_c2s_login_start : public c_packet_c2s 
{
public:
    std::string player_name;

    c_c2s_login_start() = default;

    void deserialize(c_packet& packet) override 
    {
        this->player_name = packet.read_string(16);
    }

    std::string& get_name() 
    {
        return this->player_name;
    }
};


/*
    Server to Client Packets
*/

class c_s2c_login_success : public c_packet_s2c {
public:
    std::string player_name;
    std::string player_uuid;

    c_s2c_login_success(const std::string& name, const std::string& uuid)
        : player_name(name), player_uuid(uuid) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x02);
        packet.write_string(this->player_uuid, 36);  // UUID is 36 characters
        packet.write_string(this->player_name, 16);  // Minecraft names max 16
        packet.finalize();
    }
};

class c_s2c_join_game : public c_packet_s2c {
public:
    int32_t entity_id;
    uint8_t gamemode;
    int32_t dimension;
    uint8_t difficulty;
    uint8_t max_players;
    std::string level_type;
    uint8_t reduced_debug_info;

    c_s2c_join_game(
        int32_t entity_id, uint8_t gamemode, int32_t dimension, uint8_t difficulty,
        uint8_t max_players, const std::string& level_type, uint8_t reduced_debug_info
    )
        :
        entity_id(entity_id), gamemode(gamemode), dimension(dimension), difficulty(difficulty),
        max_players(max_players), level_type(level_type), reduced_debug_info(reduced_debug_info)
    {};

    void serialize(c_packet& packet) const override 
    {
        packet.write_var_int(0x23);
        packet.write_int(this->entity_id);
        packet.write_byte(this->gamemode);
        packet.write_int(this->dimension);
        packet.write_byte(this->difficulty);
        packet.write_byte(this->max_players);
        packet.write_string(this->level_type, 16);
        packet.write_byte(this->reduced_debug_info);
        packet.finalize();
    }
};

class c_s2c_position_look : public c_packet_s2c {
public:
    double x, y, z;
    float yaw, pitch;
    uint8_t flags;
    int32_t teleport_id;

    c_s2c_position_look(double x, double y, double z, float yaw, float pitch, uint8_t flags, int32_t teleport_id)
        : x(x), y(y), z(z), yaw(yaw), pitch(pitch), flags(flags), teleport_id(teleport_id) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x2F);
        packet.write_double(this->x);
        packet.write_double(this->y);
        packet.write_double(this->z);
        packet.write_float(this->yaw);
        packet.write_float(this->pitch);
        packet.write_byte(this->flags);
        packet.write_var_int(this->teleport_id);
        packet.finalize();
    }
};

class c_s2c_chunk_data : public c_packet_s2c {
public:
    int32_t chunk_x, chunk_y;
    uint8_t ground_up_continuous;
    int32_t primary_bit_mask;
    int32_t size;
    std::vector<uint8_t> data;
    int32_t block_entity_count;
    c_s2c_chunk_data(const std::string& name, const std::string& uuid)
        : player_name(name), player_uuid(uuid) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x02);
        packet.write_string(this->player_uuid, 36);  // UUID is 36 characters
        packet.write_string(this->player_name, 16);  // Minecraft names max 16
        packet.finalize();
    }
};

#endif
