#ifndef GAME_STATE_HPP_INCLUDED
#define GAME_STATE_HPP_INCLUDED

#include <net/shared.hpp>
#include "game_modes.hpp"

struct player
{
    //int32_t player_slot = 0;
    int32_t id = 0; ///per game

    ///I don't know if I do want to store these
    ///I could carry on using the existing broadcast system
    ///but just pipe traffic through the server instead
    ///vec3f pos;
    ///vec3f rot;

    udp_sock sock;
    sockaddr_storage store;
    sf::Clock time_since_last_message;
};

namespace game_data
{
    const float round_default = 10; ///10 seconds for testing
}

struct session_state
{
    int32_t kills = 0;
    sf::Clock time_elapsed;
};


bool operator==(sockaddr_storage& s1, sockaddr_storage& s2);

///so the client will send something like
///update component playerid componentenum value
struct game_state
{
    int max_players = 10;

    int map_num = 0; ///????

    ///we really need to handle this all serverside
    ///which means the server is gunna have to keep track
    ///lets implement a simple kill counter
    game_mode_t current_game_mode; ///?
    session_state current_session_state;

    int gid = 0;

    float timeout_time_ms = 5000; ///5 seconds

    std::vector<player> player_list;

    void broadcast(const std::vector<char>& dat, int& to_skip);
    void broadcast(const std::vector<char>& dat, sockaddr_storage& to_skip);

    void cull_disconnected_players();
    void add_player(udp_sock& sock, sockaddr_storage store);

    void tick_all();

    void process_received_message(byte_fetch& fetch, sockaddr_storage& who);

    void reset_player_disconnect_timer(sockaddr_storage& store);
};


#endif // GAME_STATE_HPP_INCLUDED
