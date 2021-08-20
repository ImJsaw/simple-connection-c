#ifndef INTERCOM_TCP_SERVER_H
#define INTERCOM_TCP_SERVER_H

#include "tcp_ip.h"

#include "client.h"
#include "server_observer.h"

#define MAX_PACKET_SIZE 4096

class TcpServer
{
private:

    int m_sockfd;
    struct sockaddr_in m_serverAddress;
    struct sockaddr_in m_clientAddress;
    fd_set m_fds;
    std::vector<Client> m_clients;
    std::vector<server_observer_t> m_subscibers;
    std::thread * threadHandle;

    void publishClientMsg(const Client & client, const char * msg, size_t msgSize);
    void publishClientDisconnected(const Client & client);
    void receiveTask(/*void * context*/);


public:

    pipe_ret_t start(int port);
    Client acceptClient(uint timeout);
    bool deleteClient(Client & client);
    void subscribe(const server_observer_t & observer);
    void unsubscribeAll();
    pipe_ret_t sendToAllClients(const char * msg, size_t size);
    pipe_ret_t sendToClient(const Client & client, const char * msg, size_t size);
    pipe_ret_t finish();
    void printClients();
};



#endif //INTERCOM_TCP_SERVER_H
