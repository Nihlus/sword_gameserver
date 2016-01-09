#include <iostream>
#include <net/shared.hpp>

#include <vector>
#include "../master_server/network_messages.hpp"
#include <vec/vec.hpp>
#include "game_state.hpp"

///The code duplication and poor organisation is unreal. We need a cleanup :[
void tcp_periodic_connect(tcp_sock& sock, const std::string& address)
{
    if(sock.valid())
        return;

    static sock_info inf;

    static bool first = true; ///tries once the first time, every time

    const int delay_s = 5;

    if(inf.valid())
    {
        std::cout << "Connected to tcp server @ " << address << std::endl;

        sock = tcp_sock(inf.get());
        return;
    }

    ///If the last connection attempt has timed out, retry
    if(!inf.within_timeout() || first)
    {
        printf("Attempting connection to server\n");

        inf = tcp_connect(address, delay_s, 0);

        first = false;
    }
}

sock_info try_tcp_connect(const std::string& address, const std::string& port)
{
    sock_info inf;

    inf = tcp_connect(address, port, 1, 0);

    return inf;
}

using namespace std;

///so as it turns out, you must use canaries with tcp as its a stream protocol
///that will by why the map client doesn't work
///:[
int main(int argc, char* argv[])
{
    sock_info inf = try_tcp_connect(MASTER_IP, MASTER_PORT);
    tcp_sock to_server;

    std::string host_port = GAMESERVER_PORT;

    for(int i=1; i<argc; i++)
    {
        if(strncmp(argv[i], "-port", strlen("-port")) == 0)
        {
            if(i + 1 < argc)
            {
                host_port = argv[i+1];
            }
        }
    }

    uint32_t pnum = atoi(host_port.c_str());

    udp_sock my_server = udp_host(host_port);

    printf("Registered on port %s\n", my_server.get_host_port().c_str());

    //std::vector<tcp_sock> sockets;
    //std::vector<sockaddr_store> store_sock;

    game_state my_state;

    my_state.set_map(0);

    bool going = true;

    while(going)
    {
        if(sock_readable(to_server))
        {
            auto dat = tcp_recv(to_server);
        }

        if(!to_server.valid())
        {
            if(!inf.within_timeout())
            {
                ///resets sock info
                inf.retry();
                inf = try_tcp_connect("127.0.0.1", MASTER_PORT);

                printf("trying\n");
            }

            if(inf.valid())
            {
                to_server = tcp_sock(inf.get());

                byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::GAMESERVER);
                vec.push_back<uint32_t>(pnum);
                vec.push_back(canary_end);

                tcp_send(to_server, vec.ptr);

                printf("found master server\n");
            }

            continue;
        }

        /*tcp_sock new_fd = conditional_accept(my_server);

        if(new_fd.valid())
        {
            sockets.push_back(new_fd);
        }*/

        ///from here on, the code is totally untested
        /*for(int i=0; i<sockets.size(); i++)
        {
            tcp_sock fd = sockets[i];

            if(sock_readable(fd))
            {
                auto data = tcp_recv(fd);

                if(fd.invalid())
                {
                    sockets.erase(sockets.begin() + i);
                    i--;
                    continue;
                }

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

                    if(type == message::CLIENTJOINREQUEST)
                    {
                        int32_t found_end = fetch.get<int32_t>();

                        if(found_end != canary_end)
                            continue;

                        my_state.add_player(fd);
                        ///really need to pipe back player id

                        byte_vector vec;
                        vec.push_back(canary_start);
                        vec.push_back(message::CLIENTJOINACK);
                        vec.push_back<int32_t>(my_state.player_list.back().id);
                        vec.push_back(canary_end);

                        tcp_send(fd, vec.ptr);

                        printf("sending ack\n");

                        sockets.erase(sockets.begin() + i);
                        i--;
                        continue;
                    }
                }
                ///client pushing data to other clients
            }
        }*/

        bool any_read = true;

        while(any_read && sock_readable(my_server))
        {
            sockaddr_storage store;

            auto data = udp_receive_from(my_server, &store);

            any_read = data.size() > 0;

            byte_fetch fetch;
            fetch.ptr.swap(data);

            while(!fetch.finished() && any_read)
            {
                int32_t found_canary = fetch.get<int32_t>();

                while(found_canary != canary_start && !fetch.finished())
                {
                    found_canary = fetch.get<int32_t>();
                }

                int32_t type = fetch.get<int32_t>();

                if(type == message::CLIENTJOINREQUEST)
                {
                    my_state.process_join_request(my_server, fetch, store);
                }

                else if(type == message::FORWARDING)
                {
                    my_state.process_received_message(fetch, store);
                }

                else if(type == message::REPORT)
                {
                    my_state.process_reported_message(fetch, store);
                }
                else if(type == message::RESPAWNREQUEST)
                {
                    my_state.process_respawn_request(my_server, fetch, store);
                }
                else
                {
                    printf("err %i ", type);
                }

                //printf("client %s:%s\n", get_addr_ip(store).c_str(), get_addr_port(store).c_str());

                my_state.reset_player_disconnect_timer(store);

                /*for(auto& i : my_state.player_list)
                {
                    if(i.store == store)
                    {
                        i.time_since_last_message.restart();
                    }
                }*/
            }
                ///client pushing data to other clients
        }

        sf::sleep(sf::milliseconds(1));

        //my_state.tick_all();

        my_state.tick();

        my_state.balance_teams();

        my_state.periodic_team_broadcast();

        my_state.periodic_gamemode_stats_broadcast();

        my_state.cull_disconnected_players();
    }
}
