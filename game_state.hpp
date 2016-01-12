#ifndef GAME_STATE_HPP_INCLUDED
#define GAME_STATE_HPP_INCLUDED

#include <net/shared.hpp>
#include "game_modes.hpp"
#include <set>
#include <map>
#include "../reliability_shared.hpp"

struct player
{
    //int32_t player_slot = 0;
    int32_t id = -1; ///per game
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

bool operator==(sockaddr_storage& s1, sockaddr_storage& s2);

struct kill_count_timer
{
    std::set<int32_t> player_ids_reported_deaths; ///who reported the death ids
    sf::Clock elapsed_time_since_started;
    const float max_time = 1000.f; ///1s window
};

struct game_state;

struct game_mode_handler
{
    sf::Clock clk;

    game_mode_t current_game_mode;
    session_state current_session_state;
    session_boundaries current_session_boundaries;

    bool in_game_over_state = false;
    sf::Clock game_over_timer;

    void tick(game_state* state);

    bool game_over();
};

struct respawn_request
{
    int32_t player_id = 0;
    sf::Clock clk;
    float time_to_respawn_ms = 5000;
    float time_to_remove_ms = 10000; ///the client spams the server. Wait this much more time before they can spawn again
    bool respawned = false;
};

struct game_state;

struct server_reliability_manager
{
    std::map<int32_t, reliability_manager> player_reliability_handler;

    void tick(game_state* state);

    void add(byte_vector& vec, int32_t to_skip);
    void add_packetid_to_ack(uint32_t id, int32_t to_whom);

    void add_player(int32_t id);
    void remove_player(int32_t id);

    void process_ack(byte_fetch& fetch);
};

///so the client will send something like
///update component playerid componentenum value
struct game_state
{
    server_reliability_manager reliable;

    int max_players = 10;

    int map_num = 0; ///????
    std::vector<std::vector<vec2f>> respawn_positions;

    std::vector<respawn_request> respawn_requests;

    ///we really need to handle this all serverside
    ///which means the server is gunna have to keep track
    ///lets implement a simple kill counter
    game_mode_handler mode_handler;

    ///maps player id who died to kill count structure
    std::map<int32_t, kill_count_timer> kill_confirmer;

    int gid = 0;

    float timeout_time_ms = 5000; ///3 seconds

    ///THIS IS NOT A MAP
    ///PLAYER IDS ARE NOT POSITIONS IN THIS STRUCTURE
    std::vector<player> player_list;

    int number_of_team(int team_id);

    int32_t get_team_from_player_id(int32_t id);
    player get_player_from_player_id(int32_t id);

    void broadcast(const std::vector<char>& dat, const int& to_skip);
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
    void process_respawn_request(udp_sock& sock, byte_fetch& fetch, sockaddr_storage& who);

    void balance_teams();
    vec2f find_respawn_position(int team_id);
    void respawn_player(int32_t player_id);
    void respawn_all();

    void periodic_team_broadcast();
    void periodic_gamemode_stats_broadcast();
    void periodic_respawn_info_update();

    void reset_player_disconnect_timer(sockaddr_storage& store);

    void set_map(int);
};


#endif // GAME_STATE_HPP_INCLUDED
