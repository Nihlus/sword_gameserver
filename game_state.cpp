#include "game_state.hpp"
#include "../master_server/network_messages.hpp"

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

///need to heartbeat
void game_state::cull_disconnected_players()
{
    for(int i=0; i<player_list.size(); i++)
    {
        if(player_list[i].time_since_last_message.getElapsedTime().asMicroseconds() / 1000.f > timeout_time_ms)
        {
            printf("Player timeout\n");// %s:%s\n", get_peer_ip(player_list[i]].store))

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

void game_state::broadcast(const std::vector<char>& dat, int& to_skip)
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
            current_session_state.kills++;

            it = kill_confirmer.erase(it);

            printf("bring out your dead\n");
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
        kill_count_timer& killcount = kill_confirmer[player_id];

        int32_t who_sent_message = sockaddr_to_playerid(who);

        killcount.player_ids_reported_deaths.insert(who_sent_message);

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

void game_state::balance_teams()
{
    if(number_of_team(0) == number_of_team(1))
        return;

    ///odd number of players, rest in peace
    if(abs(number_of_team(1) - number_of_team(0)) == 1)
        return;

    int t0 = number_of_team(0);
    int t1 = number_of_team(1);

    int team_to_swap = t1 > t0 ? 1 : 0;
    int team_to_swap_to = t1 < t0 ? 1 : 0;

    int extra = abs(number_of_team(1) - number_of_team(0));

    int num_to_swap = extra / 2;

    int num_swapped = 0;

    for(auto& i : player_list)
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

            broadcast(vec.ptr, no_player);

            num_swapped++;
        }
    }
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
