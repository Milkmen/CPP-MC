#include "protocol/packets.h"
#include "protocol/player.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <map>

#ifdef _WIN32
#define _WIN32_WINNT 0x0601  // Windows 7 minimum
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#define SOCK_ERR INVALID_SOCKET
#define SOCK_ERR_VAL SOCKET_ERROR
#else
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int socket_t;
#define CLOSE_SOCKET close
#define SOCK_ERR -1
#define SOCK_ERR_VAL -1
#endif

#ifndef min
#define min std::min
#endif

constexpr uint16_t PORT = 25565;

std::map<uint32_t, c_player> connections;

void send_packet(socket_t client_fd, c_packet& packet)
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

        int sent = send(client_fd,
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

void handle_packet(c_packet& packet, socket_t client_fd, uint32_t ip)
{
    printf("=== Processing packet ===\n");
    printf("Packet size: %zu bytes\n", packet.get_size());

    try 
    {
        // Don't read the size field - it's already been stripped by the constructor
        int32_t received_id = packet.read_var_int();

        printf("Packet ID: 0x%02X\n", received_id);

        switch (received_id)
        {
        case 0x0: /* Handshake, Login, Disconnect */
        {
            if (connections.find(ip) != connections.end())
            {
                c_c2s_login_start login_start = c_c2s_login_start();
                login_start.deserialize(packet);

                connections[ip].name = login_start.player_name;

                // Login Success packet (0x02) 
                c_packet packet_out;
                c_s2c_login_success login_success = c_s2c_login_success(login_start.player_name, "123e4567-e89b-12d3-a456-426614174000");
                login_success.serialize(packet_out);
                send_packet(client_fd, packet_out);

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
                send_packet(client_fd, packet_out);

                // Player Position and Look packet (0x2F) - simplified version
                packet_out.clear();
                c_s2c_position_look pos_look = c_s2c_position_look
                (
                    0.0, 0.0, 0.0,
                    0.f, 0.f,
                    0b00000000,
                    1
                );
                pos_look.serialize(packet_out);
                send_packet(client_fd, packet_out);

                connections[ip].state = connection_state_t::play;
            }
            else
            {
                printf("Processing handshake\n");
                int32_t received_version = packet.read_var_int();
                std::string server_address = packet.read_string(255);
                uint16_t server_port = (packet.read_byte() << 8) | packet.read_byte();
                int32_t next_state = packet.read_var_int();

                printf("Protocol Version: %d\n", received_version);
                printf("Server Address: %s\n", server_address.c_str());
                printf("Server Port: %d\n", server_port);
                printf("Next State: %d\n", next_state);

                connections[ip] = c_player();
                connections[ip].state = connection_state_t::login;
            }
            break;
        }

        default:
            printf("Unknown packet ID: 0x%02X\n", received_id);
            break;
        }
    }
    catch (const std::exception& e) {
        printf("Error processing packet: %s\n", e.what());
    }

    printf("=== End packet processing ===\n\n");
}

void handle_client(socket_t client_fd, sockaddr_in client_addr) 
{
    std::vector<uint8_t> buffer(4096);
    std::vector<uint8_t> data_buf;

    int bytes_read;
    while ((bytes_read = recv(client_fd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0)) > 0)
    {
        data_buf.insert(data_buf.end(), buffer.begin(), buffer.begin() + bytes_read);

        printf("Received %d bytes, buffer size: %zu\n", bytes_read, data_buf.size());

        while (true) {
            if (data_buf.empty())
            {
                printf("Data buffer is empty, breaking\n");
                break;
            }

            // Peek into the buffer to read VarInt length
            size_t i = 0;
            int32_t length = 0;
            int shift = 0;
            bool valid_varint = false;

            while (i < data_buf.size() && shift <= 28)
            {
                uint8_t byte = data_buf[i];
                length |= (byte & 0x7F) << shift;
                shift += 7;
                i++;
                if ((byte & 0x80) == 0)
                {
                    valid_varint = true;
                    break;
                }
            }

            if (!valid_varint)
            {
                printf("Invalid or incomplete VarInt, waiting for more data\n");
                break;
            }

            if (shift > 28) 
            {
                printf("VarInt too large, disconnecting client\n");
                break;
            }

            size_t varint_len = i;

            printf("Parsed VarInt: length=%d, varint_len=%zu, buffer_size=%zu\n",
                length, varint_len, data_buf.size());

            if (length < 0 || length > 32767) 
            { // Reasonable packet size limit
                printf("Invalid packet length: %d\n", length);
                break;
            }

            if (data_buf.size() < varint_len + static_cast<size_t>(length)) 
            {
                printf("Not enough bytes for full packet: need %zu, have %zu\n",
                    varint_len + static_cast<size_t>(length), data_buf.size());
                break;
            }

            // Extract the full packet
            try 
            {
                std::vector<uint8_t> packet_bytes(data_buf.begin(),
                    data_buf.begin() + varint_len + length);
                data_buf.erase(data_buf.begin(), data_buf.begin() + varint_len + length);

                printf("Processing packet of %zu bytes\n", packet_bytes.size());

                c_packet packet(packet_bytes);
                handle_packet(packet, client_fd, client_addr.sin_addr.S_un.S_addr);
            }
            catch (const std::exception& e) 
            {
                std::cerr << "Packet parse error: " << e.what() << std::endl;
                // Print first few bytes for debugging
                printf("Problematic packet bytes: ");
                for (size_t j = 0; j < min(data_buf.size(), size_t(20)); j++)
                {
                    printf("%02X ", data_buf[j]);
                }
                printf("\n");
                break;
            }
        }
    }

    CLOSE_SOCKET(client_fd);
    std::cout << "Client disconnected." << std::endl;
}

int main() 
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == SOCK_ERR) 
    {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERR_VAL) 
    {
        std::cerr << "Bind failed" << std::endl;
        CLOSE_SOCKET(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == SOCK_ERR_VAL) 
    {
        std::cerr << "Listen failed" << std::endl;
        CLOSE_SOCKET(server_fd);
        return 1;
    }
    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) 
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd,
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_len);
        if (client_fd == SOCK_ERR) 
        {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        // Launch a thread to handle the client
        std::thread(handle_client, client_fd, client_addr).detach();
    }

    // Never reached in this example
#ifdef _WIN32
    WSACleanup();
#endif
    CLOSE_SOCKET(server_fd);
    return 0;
}
