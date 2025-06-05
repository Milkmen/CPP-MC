#include "player.h"
#include "server.h"

#include <string>
#include <sstream>

void c_player::on_handshake(c_packet& packet)
{
    switch (packet.id)
    {
    case 0x00:
    {
        c_c2s_handshake handshake = c_c2s_handshake();
        handshake.deserialize(packet);
        printf("Handshake Received with Version: %d\r\n", handshake.protocol_version);
        printf("Next State: %d\r\n", handshake.next_state);
        this->state = (connection_state_t)handshake.next_state;
        break;
    }
    }
}

void c_player::on_status(c_packet& packet)
{
    c_server* server = ((c_server*)this->server_ptr);

    switch (packet.id)
    {
    case 0x00:
    {
        c_s2c_status status = c_s2c_status(server->server_status);
        c_packet packet;
        status.serialize(packet);

        this->send_packet(packet);
        printf("Sent status: %s\r\n", server->server_status.c_str());
        break;
    }
    case 0x01:
    {
        c_c2s_ping ping = c_c2s_ping();
        ping.deserialize(packet);

        c_s2c_pong pong = c_s2c_pong(ping.time);
        c_packet pack;
        pong.serialize(pack);

        this->send_packet(pack);
        break;
    }
    }
}

void c_player::on_login(c_packet& packet)
{
    c_server* server = ((c_server*)this->server_ptr);

    switch (packet.id)
    {
    case 0x00:
    {
        c_c2s_login_start login_start = c_c2s_login_start();
        login_start.deserialize(packet);

        this->name = login_start.player_name;

        c_packet packet_out;
        c_s2c_login_success login_success = c_s2c_login_success(login_start.player_name, "123e4567-e89b-12d3-a456-426614174000");
        login_success.serialize(packet_out);
        this->send_packet(packet_out);

        entity_entry_t entry =
        {
            entity_type_t::player, this->client_fd
        };

        this->entity_id = server->entities.size();
        server->entities.push_back(entry);

        packet_out.clear();
        c_s2c_join_game join_game = c_s2c_join_game
        (
            this->entity_id,
            0,
            0,
            1,
            server->config.max_players,
            "default",
            0
        );
        join_game.serialize(packet_out);
        this->send_packet(packet_out);

        vec3d_t spawn_pos =
        {
            static_cast<double>(server->config.spawn_x),
            static_cast<double>(server->config.spawn_y),
            static_cast<double>(server->config.spawn_z)
        };

        packet_out.clear();
        c_s2c_position_look pos_look = c_s2c_position_look
        (
            spawn_pos.x, spawn_pos.y, spawn_pos.z,
            0.f, 0.f,
            0b00000000,
            1
        );
        pos_look.serialize(packet_out);
        this->send_packet(packet_out);

        this->state = connection_state_t::play;
        break;
    }
    }
}

void c_player::on_play(c_packet& packet)
{
    c_server* server = ((c_server*)this->server_ptr);

    switch (packet.id)
    {
    case 0x02:
    {
        c_c2s_chat_message chat_message = c_c2s_chat_message();
        chat_message.deserialize(packet);
        std::string msg_final = "<" + this->name + "> " + chat_message.message;
        server->broadcast(msg_final);
        break;
    }
    case 0x0D:
    {
        c_c2s_position pos = c_c2s_position();
        pos.deserialize(packet);
        this->position = { pos.x, pos.y, pos.z };
        this->on_ground = pos.on_ground;
        break;
    }
    case 0x0E:
    {
        c_c2s_position_look poslook = c_c2s_position_look();
        poslook.deserialize(packet);
        this->position = { poslook.x, poslook.y, poslook.z };
        this->rotation = { poslook.yaw,  poslook.pitch };
        this->on_ground = poslook.on_ground;
        break;
    }
    case 0x0F:
    {
        c_c2s_look look = c_c2s_look();
        look.deserialize(packet);
        this->rotation = { look.yaw,  look.pitch };
        this->on_ground = look.on_ground;
        break;
    }
    }
}

void c_player::on_receive(c_packet& packet)
{
    try
    {
        c_server* server = ((c_server*)this->server_ptr);

        switch (this->state)
        {
        case connection_state_t::handshake:
            this->on_handshake(packet);
            break;
        case connection_state_t::status:
            this->on_status(packet);
            break;
        case connection_state_t::login:
            this->on_login(packet);
            break;
        case connection_state_t::play:
            this->on_play(packet);
            break;
        }
    }
    catch (const std::exception& e) {
        printf("Error processing packet: %s\n", e.what());
    }
}

void c_player::send_message(std::string& message)
{
    c_packet packet;
    std::string chat_json = "{\"text\":\"" + message + "\"}";
    c_s2c_chat_message chat_packet = c_s2c_chat_message
    (
        chat_json,
        0
    );
    chat_packet.serialize(packet);

    this->send_packet(packet);
}

void c_player::send_packet(c_packet& packet)
{
    if (packet.get_size() <= 1) return;

    std::lock_guard<std::mutex> lock(((c_server*) this->server_ptr)->send_mutex);

    auto& out = packet.get_raw();
    printf("Sending packet of %zu bytes: ", out.size());
    for (size_t i = 0; i < min(out.size(), size_t(20)); i++) 
    {
        printf("%02X ", out[i]);
    }

    if (out.size() > 20) printf("...");
    printf("\n");

    size_t total_sent = 0;
    size_t total_size = out.size();

    while (total_sent < total_size) 
    {
        int sent = send
        (
            this->client_fd,
            reinterpret_cast<const char*>(out.data() + total_sent),
            static_cast<int>(total_size - total_sent),
            0
        );

        if (sent == SOCK_ERR_VAL) 
        {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) 
            {
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) 
            {
#endif
                printf("Socket would block\n");
                continue;
            }
            else {
#ifdef _WIN32
                printf("Send failed with error: %d\n", error);
#else
                printf("Send failed with error: %s (errno: %d)\n", strerror(errno), errno);
#endif
                return;
            }
            }
        else if (sent == 0) {
            printf("Connection closed by peer\n");
            return;
        }
        else {
            total_sent += sent;
            printf("Sent %d bytes (total: %zu/%zu)\n", sent, total_sent, total_size);
        }
        }

    printf("Successfully sent all %zu bytes\n", total_size);
}