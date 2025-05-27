#ifndef MC_PLAYER_H
#define MC_PLAYER_H

#include <vector>
#include <cstdint>
#include <stdexcept>

typedef enum
{
    login,
    play,
    status
}
connection_state_t;

class c_player
{
private:
public:
    std::string name;
    connection_state_t state;

    c_player();
};


#endif