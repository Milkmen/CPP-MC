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

class c_c2s_handshake : public c_packet_c2s
{
public:
    int32_t protocol_version;
    std::string server_address;
    uint16_t server_port;
    int32_t next_state;

    c_c2s_handshake() = default;

    void deserialize(c_packet& packet) override
    {
        this->protocol_version = packet.read_var_int();
        this->server_address = packet.read_string(255);
        this->server_port = (packet.read_byte() << 8) | packet.read_byte();
        this->next_state = packet.read_var_int();
    }
};

class c_c2s_login_start : public c_packet_c2s 
{
public:
    std::string player_name;

    c_c2s_login_start() = default;

    void deserialize(c_packet& packet) override 
    {
        this->player_name = packet.read_string(16);
    }
};

class c_c2s_chat_message : public c_packet_c2s
{
public:
    std::string message;

    c_c2s_chat_message() = default;

    void deserialize(c_packet& packet) override
    {
        this->message = packet.read_string(256);
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
    {}

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

class c_s2c_keep_alive : public c_packet_s2c {
public:
    uint64_t id;

    c_s2c_keep_alive(uint64_t id)
        : id(id) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x1F);
        packet.write_long(this->id);
        packet.finalize();
    }
};

class c_s2c_chunk_data : public c_packet_s2c {
public:
    int32_t chunk_x, chunk_y;
    uint8_t ground_up_continuous;
    int32_t primary_bit_mask;
    std::vector<uint8_t> data;
    int32_t block_entity_count;
    std::string nbt;

    c_s2c_chunk_data
    (
        int32_t chunk_x, int32_t chunk_y, uint8_t ground_up_continuous,
        int32_t primary_bit_mask, std::vector<uint8_t>& data,
        int32_t block_entity_count, std::string nbt
    )
        : chunk_x(chunk_x), chunk_y(chunk_y), ground_up_continuous(ground_up_continuous),
        primary_bit_mask(primary_bit_mask), data(data),
        block_entity_count(block_entity_count), nbt(nbt) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x20);
        packet.write_int(this->chunk_x);
        packet.write_int(this->chunk_y);
        packet.write_byte(this->ground_up_continuous);
        packet.write_var_int(this->primary_bit_mask);
        packet.write_var_int(this->data.size());
        for (int i = 0; i < this->data.size(); ++i)
            packet.write_byte(this->data.at(i));
        packet.write_var_int(this->block_entity_count);
        packet.write_nbt_string(this->nbt);
        packet.finalize();
    }
};

class c_s2c_chat_message : public c_packet_s2c {
public:
    std::string json;
    uint8_t type;

    c_s2c_chat_message(std::string& json, uint8_t type)
        : json(json), type(type) {}

    void serialize(c_packet& packet) const override {
        packet.write_var_int(0x0F);
        packet.write_string(this->json, 32767);
        packet.write_byte(this->type);
        packet.finalize();
    }
};

#endif
