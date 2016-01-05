#include <iostream>
#include <net/shared.hpp>

#include <vector>
#include "../master_server/network_messages.hpp"

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

sock_info try_tcp_connect(const std::string& address)
{
    sock_info inf;

    inf = tcp_connect(address, 1, 0);

    return inf;
}

using namespace std;

///so as it turns out, you must use canaries with tcp as its a stream protocol
///that will by why the map client doesn't work
///:[
int main()
{
    sock_info inf = try_tcp_connect("127.0.0.1");
    tcp_sock to_server;

    bool going = true;

    while(going)
    {
        if(!to_server.valid())
        {
            if(!inf.within_timeout())
            {
                ///resets sock info
                inf.retry();
                inf = try_tcp_connect("127.0.0.1");

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
        }
    }
}
