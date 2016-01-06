#ifndef GAME_STATE_HPP_INCLUDED
#define GAME_STATE_HPP_INCLUDED

#include <net/shared.hpp>

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
};

///so the client will send something like
///update component playerid componentenum value
struct game_state
{
    int max_players = 10;

    int map_num = 0; ///????

    int gid = 0;

    std::vector<player> player_list;

    void broadcast(const std::vector<char>& dat, int& to_skip);
    void broadcast(const std::vector<char>& dat, sockaddr_storage& to_skip);

    void cull_disconnected_players();
    void add_player(udp_sock& sock, sockaddr_storage store);

    void tick_all();

    void process_received_message(byte_fetch& fetch, sockaddr_storage& who);
};


#endif // GAME_STATE_HPP_INCLUDED
