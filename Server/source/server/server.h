#ifndef IMPL_SERVER_H
#define IMPL_SERVER_H

#include <stdint.h>
#include <map>

#include "network.h"

#include "../protocol/packet.h"
#include "player.h"
#include <thread>
#include <atomic>
#include <mutex>

typedef struct
{
	uint8_t max_players;
	uint16_t port;
    std::string motd;
}
server_config_t;

class c_server
{
private:
public:
	server_config_t config;
	std::atomic<bool> running = false;
	std::map<socket_t, c_player> players;
	std::vector<std::string> chat_messages;
	std::thread update_thread;
    std::mutex send_mutex;

	c_server(const char* config_name);

    c_server(c_server&& other)
        : config(other.config),
        running(other.running.load()),
        players(std::move(other.players)),
        chat_messages(std::move(other.chat_messages)),
        update_thread(std::move(other.update_thread)) {}

    c_server& operator=(c_server&& other) {
        if (this != &other) {
            config = other.config;
            running.store(other.running.load());
            players = std::move(other.players);
            chat_messages = std::move(other.chat_messages);
            if (update_thread.joinable()) {
                update_thread.join();  // or detach
            }
            update_thread = std::move(other.update_thread);
        }
        return *this;
    }

	int run();
	void loop();
	void update();
	void broadcast(std::string& message);
};

#endif