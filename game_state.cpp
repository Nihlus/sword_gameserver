#include "game_state.hpp"
#include "../master_server/network_messages.hpp"

void game_state::add_player(tcp_sock& sock)
{
    int id = gid++;

    printf("Gained a player with id %i\n", id);

    player play;
    play.id = id;
    play.sock = sock;

    player_list.push_back(play);
}

void game_state::cull_disconnected_players()
{
    for(int i=0; i<player_list.size(); i++)
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
    }
}

void game_state::broadcast(const std::vector<char>& dat, tcp_sock& to_skip)
{
    for(int i=0; i<player_list.size(); i++)
    {
        tcp_sock& fd = player_list[i].sock;

        if(fd == to_skip)
            continue;

        tcp_send(fd, dat);
    }
}

void game_state::tick_all()
{
    for(int i=0; i<player_list.size(); i++)
    {
        player& play = player_list[i];

        tcp_sock& fd = play.sock;

        if(sock_readable(fd))
        {
            auto data = tcp_recv(fd);

            if(fd.invalid())
                continue;

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
                    process_received_message(fetch, fd);
            }
        }
    }
}

void game_state::process_received_message(byte_fetch& arg, tcp_sock& who)
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

    //printf("broad\n");
    broadcast(vec.ptr, who);
}
