#include "game_state.hpp"
#include "../master_server/network_messages.hpp"

void server_reliability_manager::tick(game_state* state)
{
    const float broadcast_time_ms = 4;

    static sf::Clock clk;

    if(clk.getElapsedTime().asMilliseconds() < broadcast_time_ms)
        return;

    clk.restart();

    for(auto& i : player_reliability_handler)
    {
        int32_t id = i.first;

        player play = state->get_player_from_player_id(id);

        i.second.tick(play.sock, play.store);
    }
}

void server_reliability_manager::add(byte_vector& vec, int32_t to_skip, uint32_t reliable_id)
{
    for(auto& i : player_reliability_handler)
    {
        if(i.first == to_skip)
            continue;

        i.second.add(vec, reliable_id);
    }
}

void server_reliability_manager::add_player(int32_t id)
{
    player_reliability_handler[id];
}

void server_reliability_manager::remove_player(int32_t id)
{
    for(auto it = player_reliability_handler.begin(); it != player_reliability_handler.end();)
    {
        if(it->first == id)
        {
            it = player_reliability_handler.erase(it);
        }
        else
            ++it;
    }
}

void server_reliability_manager::process_ack(byte_fetch& fetch)
{
    for(auto& i : player_reliability_handler)
    {
        byte_fetch arg = fetch;

        i.second.process_forwarding_reliable_ack(arg);
    }

    ///id
    fetch.get<int32_t>();
    ///canary
    fetch.get<int32_t>();
}

void server_reliability_manager::add_packetid_to_ack(uint32_t reliable_id, int32_t player_id)
{
    for(auto& i : player_reliability_handler)
    {
        if(i.first == player_id)
        {
            i.second.register_ack_forwarding_reliable(reliable_id);
            return;
        }
    }
}

void game_state::add_player(udp_sock& sock, sockaddr_storage store)
{
    int id = gid++;

    printf("Gained a player with id %i\n", id);

    ///last message time set to 0
    player play;
    play.id = id;
    play.sock = sock;
    play.store = store;

    player_list.push_back(play);

    reliable.add_player(id);
}

int game_state::number_of_team(int team_id)
{
    int c = 0;

    for(auto& i : player_list)
    {
        if(i.team == team_id)
            c++;
    }

    return c;
}

int32_t game_state::get_team_from_player_id(int32_t id)
{
    for(auto& i : player_list)
    {
        if(i.id == id)
            return i.team;
    }

    return -1;
}

player game_state::get_player_from_player_id(int32_t id)
{
    for(auto& i : player_list)
    {
        if(i.id == id)
            return i;
    }

    return player();
}

///need to heartbeat
void game_state::cull_disconnected_players()
{
    for(int i=0; i<player_list.size(); i++)
    {
        if(player_list[i].time_since_last_message.getElapsedTime().asMicroseconds() / 1000.f > timeout_time_ms)
        {
            printf("Player timeout\n");// %s:%s\n", get_peer_ip(player_list[i]].store))

            reliable.remove_player(player_list[i].id);

            player_list.erase(player_list.begin() + i);
            i--;
        }
    }
}

bool operator==(sockaddr_storage& s1, sockaddr_storage& s2)
{
    sockaddr_in* si1 = (sockaddr_in*)&s1;
    sockaddr_in* si2 = (sockaddr_in*)&s2;

    if(si1->sin_port == si2->sin_port &&
       si1->sin_addr.s_addr == si2->sin_addr.s_addr)
        return true;

    return false;
}

void game_state::reset_player_disconnect_timer(sockaddr_storage& store)
{
    for(auto& i : player_list)
    {
        if(i.store == store)
        {
            i.time_since_last_message.restart();
        }
    }
}

void game_state::set_map(int id)
{
    map_num = id;

    respawn_positions.clear();

    for(auto& i : map_namespace::team_list)
    {
        respawn_positions.push_back(game_map::get_spawn_positions(i, map_num));
    }

    if(respawn_positions.size() < TEAM_NUMS)
    {
        int old_size = respawn_positions.size();

        for(int i=respawn_positions.size(); i<TEAM_NUMS; i++)
        {
            int id = i % old_size;

            respawn_positions.push_back(respawn_positions[id]);
        }
    }
}

void game_state::broadcast(const std::vector<char>& dat, const int& to_skip)
{
    for(int i=0; i<player_list.size(); i++)
    {
        udp_sock& fd = player_list[i].sock;
        sockaddr_storage store = player_list[i].store;

        if(i == to_skip)
            continue;

        udp_send_to(fd, dat, (sockaddr*)&store);
    }
}

void game_state::broadcast(const std::vector<char>& dat, sockaddr_storage& to_skip)
{
    int c = 0;

    for(int i=0; i<player_list.size(); i++)
    {
        udp_sock& fd = player_list[i].sock;
        sockaddr_storage store = player_list[i].store;

        if(store == to_skip)
        {
            c++;
            continue;
        }

        udp_send_to(fd, dat, (sockaddr*)&store);
    }

    if(c > 1)
        printf("ip conflict ");
}

#if 0
void game_state::tick_all()
{
    for(int i=0; i<player_list.size(); i++)
    {
        player& play = player_list[i];

        udp_sock& fd = play.sock;

        bool any_read = true;

        while(any_read && sock_readable(fd))
        {
            sockaddr_storage store;

            auto data = udp_receive_from(fd, &store);

            any_read = data.size() > 0;

            if(fd.invalid())
                continue;

            /*int which_player = -1;

            for(int j=0; j<player_list.size(); j++)
            {
                if(player_list[j].store == store)
                {
                    which_player = j;

                    //printf("received from %i\n", j);
                }
            }*/

            //printf("F %i\n", which_player);

            byte_fetch fetch;
            fetch.ptr.swap(data);

            while(!fetch.finished())
            {
                int32_t found_canary = fetch.get<int32_t>();

                while(found_canary != canary_start && !fetch.finished())
                {
                    found_canary = fetch.get<int32_t>();
                }

                int32_t type = fetch.get<int32_t>();

                if(type == message::FORWARDING)
                    process_received_message(fetch, store);
            }
        }
    }
}
#endif

int32_t game_state::sockaddr_to_playerid(sockaddr_storage& who)
{
    for(auto& i : player_list)
    {
        if(i.store == who)
            return i.id;
    }

    return -1;
}

///do kill confirmer updates here
void game_state::tick()
{
    const float player_kill_confirm = 0.8;

    int players_needed_to_confirm_kill = (player_list.size() * player_kill_confirm);

    for(auto it = kill_confirmer.begin(); it != kill_confirmer.end();)
    {
        int32_t who_is_reported_dead = it->first;

        kill_count_timer ktime = it->second;

        ///if the timer elapsed and there's enough reports
        ///he's dead
        ///prevents more reports than necessary breaking stuff
        if(ktime.player_ids_reported_deaths.size() >= players_needed_to_confirm_kill &&
           ktime.elapsed_time_since_started.getElapsedTime().asMicroseconds() / 1000.f > ktime.max_time)
        {
            ///gunna need to do teams later!
            //mode_handler.current_session_state.kills++;

            int32_t team = get_team_from_player_id(who_is_reported_dead);

            if(team >= TEAM_NUMS)
            {
                printf("team error in server\n");
                continue;
            }

            int32_t player_who_killed_them = -1;

            for(auto& i : ktime.player_id_of_reported_killer)
            {
                if(i.second >= players_needed_to_confirm_kill)
                {
                    player_who_killed_them = i.first;
                }
            }

            if(player_who_killed_them == -1)
            {
                printf("no player id could be reliably found who killed player with id %i\n", who_is_reported_dead);
            }

            int32_t killer_team = get_team_from_player_id(player_who_killed_them);

            if(killer_team >= TEAM_NUMS)
            {
                printf("invalid team for killer\n");
            }

            mode_handler.current_session_state.team_killed[team]++;

            if(killer_team >= 0 && killer_team < TEAM_NUMS)
                mode_handler.current_session_state.team_kills[killer_team]++;

            it = kill_confirmer.erase(it);

            //printf("bring out your dead, member of team %i died\n", team);
            printf("Team %i killed member of team %i\n", killer_team, team);
            printf("playerid %i killed playerid %i\n", who_is_reported_dead, player_who_killed_them);
        }
        else
        if(ktime.elapsed_time_since_started.getElapsedTime().asMicroseconds() / 1000.f > ktime.max_time)
        {
            it = kill_confirmer.erase(it);

            ///well, this cant deal with hacks
            ///only packet loss
            ///as hp is authoritative from the client
            ///so if I hit you, itll definitely register
            ///unless the packet is gone
            ///but later we can extend the principle of this mechanism
            printf("hacks or packet loss\n");
        }
        else
            it++;
    }

    mode_handler.tick(this);

    for(int i=0; i<respawn_requests.size(); i++)
    {
        respawn_request& req = respawn_requests[i];

        if(req.clk.getElapsedTime().asMilliseconds() > req.time_to_respawn_ms && !req.respawned)
        {
            respawn_player(req.player_id);

            printf("respawning a player with id %i\n", req.player_id);

            req.respawned = true;
        }

        if(req.clk.getElapsedTime().asMilliseconds() > req.time_to_remove_ms)
        {
            printf("Player with id %i can respawn again\n", req.player_id);

            respawn_requests.erase(respawn_requests.begin() + i);
            i--;
        }
    }
}

void game_state::process_received_message(byte_fetch& arg, sockaddr_storage& who)
{
    ///copy
    byte_fetch fetch = arg;

    ///structure of a forwarding request

    ///canary ///already popped by calling function
    ///type ///already popped from parent function
    ///int32_t playerid
    ///int32_t componentid
    ///int32_t byte length of payload
    ///payload
    ///canary

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::FORWARDING);
    vec.push_back<int32_t>(fetch.get<int32_t>()); ///playerid
    vec.push_back<int32_t>(fetch.get<int32_t>()); ///componentid

    ///length of payload
    int32_t len = fetch.get<int32_t>();

    vec.push_back<int32_t>(len);

    for(int i=0; i<len; i++)
    {
        vec.push_back<uint8_t>(fetch.get<uint8_t>());
    }

    int32_t found_end = fetch.get<int32_t>();

    ///rewind
    if(found_end != canary_end)
    {
        printf("canary mismatch in processed received message\n");
        return;
    }

    vec.push_back(canary_end);

    arg = fetch;

    broadcast(vec.ptr, who);
}

void game_state::process_reported_message(byte_fetch& arg, sockaddr_storage& who)
{
    byte_fetch fetch = arg;

    ///canary_start
    ///message::REPORT
    ///TYPE ///so we start here
    ///PLAYERID ///optional?
    ///LEN
    ///DATA
    ///canary_end

    int32_t report_type = fetch.get<int32_t>();
    int32_t player_id = fetch.get<int32_t>(); ///who is reported dead

    int32_t len = fetch.get<int32_t>();

    std::vector<char> dat;

    for(int i=0; i<len; i++)
    {
        dat.push_back(fetch.get<uint8_t>());
    }

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        printf("canary mismatch in processed reported message\n");
        return;
    }

    if(report_type == (int32_t)report::DEATH)
    {
        if(dat.size() < sizeof(int32_t))
        {
            printf("Err, invalid death report\n");
            return;
        }

        int32_t player_who_killed_them = *(int32_t*)&dat[0];

        kill_count_timer& killcount = kill_confirmer[player_id];

        int32_t who_sent_message = sockaddr_to_playerid(who);

        killcount.player_ids_reported_deaths.insert(who_sent_message);
        killcount.player_id_of_reported_killer[player_who_killed_them]++;

        if(player_who_killed_them == -1)
        {
            printf("warning, bad player who killed them id = -1\n");
        }

        printf("Kill reported\n");
    }

    arg = fetch;
}

void game_state::process_join_request(udp_sock& my_server, byte_fetch& fetch, sockaddr_storage& who)
{
    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
        return;

    printf("Player joined %s:%s\n", get_addr_ip(who).c_str(), get_addr_port(who).c_str());

    add_player(my_server, who);
    ///really need to pipe back player id

    ///should really dynamically organise teams
    ///so actually that's what I'll do
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::CLIENTJOINACK);
    vec.push_back<int32_t>(player_list.back().id);
    vec.push_back(canary_end);

    udp_send_to(my_server, vec.ptr, (const sockaddr*)&who);

    printf("sending ack\n");
}

void game_state::process_respawn_request(udp_sock& my_server, byte_fetch& fetch, sockaddr_storage& who)
{
    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
        return;

    ///THIS IS NOT THE POSITION IN THE PLAYER STRUCTURE
    int player_id = sockaddr_to_playerid(who);

    for(auto& i : respawn_requests)
    {
        if(i.player_id == player_id)
            return;
    }

    respawn_request req;
    req.player_id = player_id;

    respawn_requests.push_back(req);
}

void game_state::process_ping_response(udp_sock& my_server, byte_fetch& fetch, sockaddr_storage& who)
{
    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
        return;

    int32_t player_id = sockaddr_to_playerid(who);

    if(player_id < 0)
    {
        printf("Invalid player id %i\n", player_id);
        return;
    }

    player& play = player_list[player_id];

    float ctime = running_time.getElapsedTime().asMicroseconds() / 1000.f;

    play.ping_ms = ctime - last_time_ms;

    //if(play.ping_ms > play.max_ping_ms)
    //    play.ping_ms = play.max_ping_ms;

    printf("Ping %f %i\n", play.ping_ms, play.id);

    //printf("running time %f\n", ctime);
    //printf("last time %f\n", last_time_ms);
}

///ok, the server can store everyone's pings and then distribute to clients
///really we should be sending out timestamps with all the updates, and then use that :[

void game_state::ping()
{
    //for(auto& i : player_list)
    //    i.clk.restart();

    last_time_ms = running_time.getElapsedTime().asMicroseconds() / 1000.f;

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::PING);
    vec.push_back(canary_end);

    int none = -1;

    broadcast(vec.ptr, none);

    //printf("running time2 %f\n", last_time_ms);

    //printf("sping\n");
}

///oh dear. Ping doesn't want to be a global thing :[
/*void game_state::process_ping_and_forward(udp_sock& my_server, byte_fetch& fetch, sockaddr_storage& who)
{
    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
        return;

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::PING);
    vec.push_back(canary_end);

    int none = -1;

    broadcast(vec.ptr, none);
}

void game_state::process_ping_response_and_forward(udp_sock& my_server, byte_fetch& fetch, sockaddr_storage& who)
{
    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
        return;

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::PING);
    vec.push_back(canary_end);

    int none = -1;

    broadcast(vec.ptr, none);
}*/

void game_state::respawn_player(int32_t player_id)
{
    int team_id = get_team_from_player_id(player_id);

    bool found = false;

    player play;

    for(auto& i : player_list)
    {
        if(i.id == player_id)
        {
            play = i;

            found = true;
        }
    }

    if(!found)
    {
        printf("No player with id %i\n", player_id);
        return;
    }

    vec2f new_pos = find_respawn_position(team_id);

    printf("Got teamid %i\n", team_id);

    ///I could structure this into a forward request
    ///but that'd be a pain
    ///unsafe, relying on client to update information to peers
    ///however, not unreliable, as the client will keep requesting
    ///a respawn until it gets one, and udp delivered whole
    ///except freak occurence
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::RESPAWNRESPONSE);
    vec.push_back(new_pos);
    vec.push_back(canary_end);

    udp_send_to(play.sock, vec.ptr, (const sockaddr*)&play.store);
}

vec2f game_state::find_respawn_position(int team_id)
{
    if(respawn_positions.size() <= team_id)
    {
        printf("Respawn_positions too small, returning default spawn\n");

        return {-3, -3};
    }

    ///I think I'm going to hate this function
    std::vector<vec2f>& valid_spawns = respawn_positions[team_id];

    if(valid_spawns.size() == 0)
    {
        printf("No valid spawns for this team\n");

        return {-1, -1};
    }

    if(team_id < 0 || team_id >= valid_spawns.size())
    {
        printf("Error, invalid team id with %i\n", team_id);

        return {-2, -2};
    }

    vec2f my_spawn = valid_spawns.front();

    ///well uuh hi. This erases the first member of the vector
    ///and pushes it to the back
    ///this is not exactly ideal from a programming front
    ///but hopefully this bit of sellotape should work out just fine
    valid_spawns.erase(valid_spawns.begin());
    valid_spawns.push_back(my_spawn);

    return my_spawn;
}

void balance_first_to_x(game_state& state)
{
    if(state.number_of_team(0) == state.number_of_team(1))
        return;

    ///odd number of players, rest in peace
    if(abs(state.number_of_team(1) - state.number_of_team(0)) == 1)
        return;

    int t0 = state.number_of_team(0);
    int t1 = state.number_of_team(1);

    int team_to_swap = t1 > t0 ? 1 : 0;
    int team_to_swap_to = t1 < t0 ? 1 : 0;

    int extra = abs(state.number_of_team(1) - state.number_of_team(0));

    int num_to_swap = extra / 2;

    int num_swapped = 0;

    for(auto& i : state.player_list)
    {
        if(i.team == team_to_swap && num_swapped < num_to_swap)
        {
            i.team = team_to_swap_to;
            ///network

            byte_vector vec;
            vec.push_back(canary_start);
            vec.push_back(message::TEAMASSIGNMENT);
            vec.push_back<int32_t>(i.id);
            vec.push_back<int32_t>(i.team);
            vec.push_back(canary_end);

            //udp_send_to(i.sock, vec.ptr, (const sockaddr*)&i.store);

            int no_player = -1;

            state.broadcast(vec.ptr, no_player);

            num_swapped++;
        }
    }
}

///well, more of an iterative balance. Should probably make it properly fully balance
void balance_ffa(game_state& state)
{
    int number_of_players = state.player_list.size();

    int max_players_per_team = (number_of_players / TEAM_NUMS) + 1;

    std::vector<int32_t> too_big_teams;
    too_big_teams.resize(TEAM_NUMS);

    for(int i=0; i<TEAM_NUMS; i++)
    {
        if(state.number_of_team(i) > max_players_per_team)
        {
            /*int32_t next_team = (i+1) % TEAM_NUMS;

            byte_vector vec;

            vec.push_back(canary_start);
            vec.push_back(message::TEAMASSIGNMENT);
            vec.push_back<int32_t>()*/

            too_big_teams[i] = 1;
        }
    }

    for(auto& i : state.player_list)
    {
        if(too_big_teams[i.team] == 1)
        {
            int old_team = i.team;

            i.team = (i.team + 1) % TEAM_NUMS;

            byte_vector vec;
            vec.push_back(canary_start);
            vec.push_back(message::TEAMASSIGNMENT);
            vec.push_back<int32_t>(i.id);
            vec.push_back<int32_t>(i.team);
            vec.push_back(canary_end);

            int no_player = -1;

            state.broadcast(vec.ptr, no_player);

            too_big_teams[old_team] = 0;
        }
    }
}

void game_state::balance_teams()
{
    if(mode_handler.current_game_mode == game_mode::FIRST_TO_X)
        return balance_first_to_x(*this);

    if(mode_handler.current_game_mode == game_mode::FFA)
        return balance_ffa(*this);
}

void game_state::periodic_team_broadcast()
{
    static sf::Clock clk;

    ///once per second
    float broadcast_every_ms = 1000.f;

    if(clk.getElapsedTime().asMicroseconds() / 1000.f < broadcast_every_ms)
        return;

    clk.restart();

    for(auto& i : player_list)
    {
        ///network
        byte_vector vec;
        vec.push_back(canary_start);
        vec.push_back(message::TEAMASSIGNMENT);
        vec.push_back<int32_t>(i.id);
        vec.push_back<int32_t>(i.team);
        vec.push_back(canary_end);

        int no_player = -1;

        broadcast(vec.ptr, no_player);
    }
}

void game_state::periodic_gamemode_stats_broadcast()
{
    static sf::Clock clk;

    ///once per second
    float broadcast_every_ms = 1000.f;

    if(clk.getElapsedTime().asMicroseconds() / 1000.f < broadcast_every_ms)
        return;

    clk.restart();

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::GAMEMODEUPDATE);
    vec.push_back(mode_handler.current_game_mode);

    vec.push_back(mode_handler.current_session_state);
    vec.push_back(mode_handler.current_session_boundaries);

    vec.push_back(canary_end);

    broadcast(vec.ptr, -1);
}

void game_state::periodic_respawn_info_update()
{
    static sf::Clock clk;

    ///once per second
    float broadcast_every_ms = 100.f;

    if(clk.getElapsedTime().asMicroseconds() / 1000.f < broadcast_every_ms)
        return;

    clk.restart();

    for(auto& i : respawn_requests)
    {
        player play = get_player_from_player_id(i.player_id);

        if(play.id == -1)
            continue;

        float time_elapsed = i.clk.getElapsedTime().asMicroseconds() / 1000.f;

        byte_vector vec;
        vec.push_back(canary_start);
        vec.push_back(message::RESPAWNINFO);
        vec.push_back(time_elapsed);
        vec.push_back(i.time_to_respawn_ms);
        vec.push_back(canary_end);

        udp_send_to(play.sock, vec.ptr, (const sockaddr*)&play.store);
    }
}

void game_mode_handler::tick(game_state* state)
{
    current_session_state.time_elapsed += clk.getElapsedTime().asMicroseconds() / 1000.f;
    clk.restart();

    if(game_over())
    {
        ///just changed
        if(!in_game_over_state)
        {
            game_over_timer.restart();

            printf("Round end\n");
        }

        in_game_over_state = true;
    }

    ///we'd need to update all the client positions here
    if(in_game_over_state && game_over_timer.getElapsedTime().asSeconds() > game_data::game_over_time)
    {
        ///reset the session state
        current_session_state = session_state();

        in_game_over_state = false;

        state->respawn_requests.clear();

        printf("Round begin\n");
    }
}

bool game_mode_handler::game_over()
{
    /*return current_session_state.team_0_killed >= current_session_boundaries.max_kills ||
            current_session_state.team_1_killed >= current_session_boundaries.max_kills ||
            current_session_state.time_elapsed >= current_session_boundaries.max_time_ms;*/

    /*for(int i=0; i<TEAM_NUMS; i++)
    {
        ///WARNING, THIS SHOULD BE TEAM KILLS
        if(current_session_state.team_kills[i] >= current_session_boundaries.max_kills)
            return true;
    }

    if(current_session_state.time_elapsed >= current_session_boundaries.max_time_ms)
        return true;

    return false;*/

    return current_session_state.game_over(current_session_boundaries);
}
