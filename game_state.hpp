#ifndef GAME_STATE_HPP_INCLUDED
#define GAME_STATE_HPP_INCLUDED

#include <net/shared.hpp>
#include "game_modes.hpp"
#include <set>
#include <map>

struct player
{
    //int32_t player_slot = 0;
    int32_t id = 0; ///per game
    int32_t team = 0;

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

struct kill_count_timer
{
    std::set<int32_t> player_ids_reported_deaths; ///who reported the death ids
    sf::Clock elapsed_time_since_started;
    const float max_time = 1000.f; ///1s window
};

///so the client will send something like
///update component playerid componentenum value
struct game_state
{
    int max_players = 10;
    //int num_players = 0;

    int map_num = 0; ///????

    ///we really need to handle this all serverside
    ///which means the server is gunna have to keep track
    ///lets implement a simple kill counter
    game_mode_t current_game_mode; ///?
    session_state current_session_state;

    ///maps player id who died to kill count structure
    std::map<int32_t, kill_count_timer> kill_confirmer;

    int gid = 0;

    float timeout_time_ms = 3000; ///3 seconds

    std::vector<player> player_list;

    int number_of_team(int team_id);

    void broadcast(const std::vector<char>& dat, int& to_skip);
    void broadcast(const std::vector<char>& dat, sockaddr_storage& to_skip);

    void cull_disconnected_players();
    void add_player(udp_sock& sock, sockaddr_storage store);

    ///for the moment just suck at it

    int32_t sockaddr_to_playerid(sockaddr_storage& who);

    //void tick_all();

    void tick();

    void process_received_message(byte_fetch& fetch, sockaddr_storage& who);
    void process_reported_message(byte_fetch& fetch, sockaddr_storage& who);
    void process_join_request(udp_sock& sock, byte_fetch& fetch, sockaddr_storage& who);

    void balance_teams();

    void periodic_team_broadcast();

    void reset_player_disconnect_timer(sockaddr_storage& store);
};


#endif // GAME_STATE_HPP_INCLUDED
