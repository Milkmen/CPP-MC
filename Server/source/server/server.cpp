#include "server.h"

#include "../protocol/packets.h"

#include <SimpleIni.h>

c_server::c_server(const char* config_name)
{
	CSimpleIniA ini;
	ini.LoadFile(config_name);

	long port					= ini.GetLongValue("Server", "port", 25565);
	long max_players			= ini.GetLongValue("Server", "max_players", 16);

	this->config.port			= port > UINT16_MAX ? UINT16_MAX : port;
	this->config.max_players	= max_players > UINT8_MAX ? UINT8_MAX : max_players;

	printf("Port: %d\n", this->config.port);
	printf("Max Players: %d\n", this->config.max_players);
}

int c_server::run()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed\r\n");
        return 1;
    }
#endif

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == SOCK_ERR)
    {
        printf("Socket creation failed\r\n");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(this->config.port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERR_VAL)
    {
        printf("Bind failed\r\n");
        CLOSE_SOCKET(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == SOCK_ERR_VAL)
    {
        printf("Listen failed\r\n");
        CLOSE_SOCKET(server_fd);
        return 1;
    }

    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int max_fd = server_fd;

    std::map<socket_t, std::vector<uint8_t>> client_buffers;

    while (true) {
        read_fds = master_set; // Copy the set

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            printf("select() failed\r\n");
            break;
        }

        for (int fd = 0; fd <= max_fd; ++fd) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            if (fd == server_fd) {
                // New client
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                socket_t client_fd = accept(server_fd,
                    reinterpret_cast<sockaddr*>(&client_addr),
                    &client_len);
                if (client_fd != SOCK_ERR) {
#ifdef _WIN32
                    u_long mode = 1;
                    ioctlsocket(client_fd, FIONBIO, &mode);
#else
                    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
#endif
                    FD_SET(client_fd, &master_set);
                    if (client_fd > max_fd) max_fd = client_fd;
                    client_buffers[client_fd] = {};
                    printf("Client connected \r\n");
                }
            }
            else {
                // Existing client data
                std::vector<uint8_t> buffer(4096);
                int bytes_read = recv(fd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
                if (bytes_read <= 0) {
                    printf("Client disconnected\r\n");
                    CLOSE_SOCKET(fd);
                    FD_CLR(fd, &master_set);
                    client_buffers.erase(fd);
                }
                else {
                    auto& data_buf = client_buffers[fd];
                    data_buf.insert(data_buf.end(), buffer.begin(), buffer.begin() + bytes_read);

                    // Inline packet processing logic
                    while (true) {
                        if (data_buf.empty()) break;

                        size_t i = 0;
                        int32_t length = 0;
                        int shift = 0;
                        bool valid_varint = false;

                        while (i < data_buf.size() && shift <= 28) {
                            uint8_t byte = data_buf[i];
                            length |= (byte & 0x7F) << shift;
                            shift += 7;
                            i++;
                            if ((byte & 0x80) == 0) {
                                valid_varint = true;
                                break;
                            }
                        }

                        if (!valid_varint || shift > 28) break;
                        size_t varint_len = i;

                        if (data_buf.size() < varint_len + static_cast<size_t>(length)) break;

                        try {
                            std::vector<uint8_t> packet_bytes(
                                data_buf.begin(),
                                data_buf.begin() + varint_len + length);
                            data_buf.erase(data_buf.begin(), data_buf.begin() + varint_len + length);

                            c_packet packet(packet_bytes);
                            packet.id = packet.read_var_int();

                            if (players.find(fd) == players.end()) {
                                this->players[fd].client_fd = fd;
                            }

                            this->players[fd].on_receive(packet);
                        }
                        catch (const std::exception& e) {
                            printf("Error: %s\r\n", e.what());
                            break;
                        }
                    }
                }
            }
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif
    CLOSE_SOCKET(server_fd);
    return 0;
}
