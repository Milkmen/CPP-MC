#include "player.h"
#include "server.h"

void c_player::on_receive(c_packet& packet)
{
    try
    {
        switch (packet.id)
        {
        case 0x0:
        {
            if (this->state == connection_state_t::handshake)
            {
                c_c2s_handshake handshake = c_c2s_handshake();
                handshake.deserialize(packet);
                printf("Handshake Received with Version: %d\r\n", handshake.protocol_version);
                printf("Next State: %d\r\n", handshake.next_state);
                this->state = (connection_state_t) handshake.next_state;
                break;
            }

            if (this->state == connection_state_t::login)
            {
                c_c2s_login_start login_start = c_c2s_login_start();
                login_start.deserialize(packet);

                this->name = login_start.player_name;

                c_packet packet_out;
                c_s2c_login_success login_success = c_s2c_login_success(login_start.player_name, "123e4567-e89b-12d3-a456-426614174000");
                login_success.serialize(packet_out);
                this->send_packet(packet_out);

                packet_out.clear();
                c_s2c_join_game join_game = c_s2c_join_game
                (
                    1,
                    0,
                    0,
                    1,
                    16,
                    "default",
                    0
                );
                join_game.serialize(packet_out);
                this->send_packet(packet_out);

                packet_out.clear();
                c_s2c_position_look pos_look = c_s2c_position_look
                (
                    0.0, 64.0, 0.0,
                    0.f, 0.f,
                    0b00000000,
                    1
                );
                pos_look.serialize(packet_out);
                this->send_packet(packet_out);

                this->state = connection_state_t::play;
            }
            else if (this->state == connection_state_t::status)
            {
                std::string statjson = "{\"description\":{\"text\": \"" + ((c_server*)this->server_ptr)->config.motd + "\"}}";
                c_s2c_status status = c_s2c_status(statjson);
                c_packet packet;
                status.serialize(packet);

                this->send_packet(packet);
                printf("Sent status: %s\r\n", statjson.c_str());
            }
            
            break;
        }
        case 0x01:
        {
            if (this->state == connection_state_t::status)
            {
                c_c2s_ping ping = c_c2s_ping();
                ping.deserialize(packet);

                c_s2c_pong pong = c_s2c_pong(ping.time);
                c_packet pack;
                pong.serialize(pack);

                this->send_packet(pack);
            }
            break;
        }
        case 0x02:
        {
            c_c2s_chat_message chat_message = c_c2s_chat_message();
            chat_message.deserialize(packet);
            std::string msg_final = "<" + this->name + "> " + chat_message.message;
            ((c_server*)this->server_ptr)->broadcast(msg_final);
            break;
        }
        default:
            printf("Unknown packet ID: 0x%02X\n", packet.id);
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
    for (size_t i = 0; i < min(out.size(), size_t(20)); i++) {
        printf("%02X ", out[i]);
    }
    if (out.size() > 20) printf("...");
    printf("\n");

    size_t total_sent = 0;
    size_t total_size = out.size();

    while (total_sent < total_size) {
        int sent = send(this->client_fd,
            reinterpret_cast<const char*>(out.data() + total_sent),
            static_cast<int>(total_size - total_sent),
            0);

        if (sent == SOCK_ERR_VAL) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
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