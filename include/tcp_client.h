//
// Created by erauper on 4/7/19.
//

#ifndef INTERCOM_TCP_CLIENT_H
#define INTERCOM_TCP_CLIENT_H


#include "tcp_ip.h"
#include "client_observer.h"

#define MAX_PACKET_SIZE 4096

using namespace std;

//TODO: REMOVE ABOVE CODE, AND SHARE client.h FILE WITH SERVER AND CLIENT

class TcpClient
{
private:
    int m_sockfd = 0;
    bool stop = false;
    struct sockaddr_in m_server;
    std::vector<client_observer_t> m_subscibers;
    std::thread * m_receiveTask = nullptr;
    string server_ip;

    void publishServerMsg(const char * msg, size_t msgSize);
    void publishServerDisconnected(const pipe_ret_t & ret);
    void ReceiveTask();
    void terminateReceiveThread();

    pipe_ret_t sendMsg(std::string msg, size_t size);
    pipe_ret_t connectTo(const std::string& address, int port);

public:
    TcpClient(string ip, incoming_packet_func_t recieveFunc, disconnected_func_t disconnectFunc);
    TcpClient();
    ~TcpClient();
    

    bool connectServer(int port);

    bool sendToServer(string msg);

    void subscribe(const client_observer_t & observer);
    void unsubscribeAll();

    pipe_ret_t finish();
};

#endif //INTERCOM_TCP_CLIENT_H
