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
int main()
{
    sock_info inf = try_tcp_connect("127.0.0.1", MASTER_PORT);
    tcp_sock to_server;

    tcp_sock my_server = tcp_host(GAMESERVER_PORT);

    std::vector<tcp_sock> sockets;

    game_state my_state;

    bool going = true;

    while(going)
    {
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
                vec.push_back(canary_end);

                tcp_send(to_server, vec.ptr);

                printf("found master server\n");
            }

            continue;
        }

        tcp_sock new_fd = conditional_accept(my_server);

        if(new_fd.valid())
        {
            sockets.push_back(new_fd);
        }

        for(int i=0; i<sockets.size(); i++)
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

                    sockets.erase(sockets.begin() + i);
                    i--;
                    continue;
                }

                ///client pushing data to other clients
            }
        }

        my_state.tick_all();

        my_state.cull_disconnected_players();
    }
}
