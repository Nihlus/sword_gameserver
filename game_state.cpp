#include "game_state.hpp"
#include "../master_server/network_messages.hpp"

void game_state::add_player(udp_sock& sock, sockaddr_storage store)
{
    int id = gid++;

    printf("Gained a player with id %i\n", id);

    player play;
    play.id = id;
    play.sock = sock;
    play.store = store;

    player_list.push_back(play);
}

void game_state::cull_disconnected_players()
{
    /*for(int i=0; i<player_list.size(); i++)
    {
        tcp_sock fd = player_list[i].sock;

        if(sock_readable(fd))
        {
            auto data = tcp_recv(fd);

            if(fd.invalid())
            {
                player_list[i].sock.close();
            }
        }

        if(!player_list[i].sock.valid())
        {
            printf("player died\n");

            player_list[i].sock.close();

            player_list.erase(player_list.begin() + i);
            i--;
        }
    }*/
}

bool operator==(sockaddr_storage& s1, sockaddr_storage& s2)
{
    //if(memcmp(&s1, &s2, sizeof(s1)) == 0)
    //    return true;

    sockaddr_in* si1 = (sockaddr_in*)&s1;
    sockaddr_in* si2 = (sockaddr_in*)&s2;

    if(si1->sin_port == si2->sin_port &&
       si1->sin_addr.s_addr == si2->sin_addr.s_addr)
        return true;

    return false;
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
        printf("ip conflict\n");
}

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

            int which_player = -1;

            for(int j=0; j<player_list.size(); j++)
            {
                if(player_list[j].store == store)
                {
                    which_player = j;

                    //printf("received from %i\n", j);
                }
            }

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
