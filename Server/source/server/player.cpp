#include "player.h"

void c_player::on_receive(c_packet& packet)
{
    try
    {
        switch (packet.id)
        {
        case 0x0:
        {
            if (this->state != connection_state_t::invalid)
            {
                c_c2s_login_start login_start = c_c2s_login_start();
                login_start.deserialize(packet);

                this->name = login_start.player_name;

                // Login Success packet (0x02)
                c_packet packet_out;
                c_s2c_login_success login_success = c_s2c_login_success(login_start.player_name, "123e4567-e89b-12d3-a456-426614174000");
                login_success.serialize(packet_out);
                this->send_packet(packet_out);

                // Join Game packet (0x23)
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

                /* Player Positionand Look packet(0x2F)*/
                packet_out.clear();
                c_s2c_position_look pos_look = c_s2c_position_look
                (
                    0.0, 0.0, 0.0,
                    0.f, 0.f,
                    0b00000000,
                    1
                );
                pos_look.serialize(packet_out);
                this->send_packet(packet_out);

                this->state = connection_state_t::login;
            }
            else
            {
                c_c2s_handshake handshake = c_c2s_handshake();
                handshake.deserialize(packet);
                printf("Handshake Received with Version: %d\r\n", handshake.protocol_version);
                this->state = connection_state_t::login;
            }
            
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

void c_player::send_packet(c_packet& packet)
{
    if (packet.get_size() > 1)
    {
        auto& out = packet.get_raw();
        printf("Sending packet of %zu bytes: ", out.size());
        for (size_t i = 0; i < min(out.size(), size_t(20)); i++) {
            printf("%02X ", out[i]);
        }
        if (out.size() > 20) printf("...");
        printf("\n");

        int sent = send(this->connection.client_fd,
            reinterpret_cast<const char*>(out.data()),
            static_cast<int>(out.size()),
            0);

        if (sent == SOCK_ERR_VAL) {
            printf("Send failed with error\n");
        }
        else {
            printf("Successfully sent %d bytes\n", sent);
        }
    }
}