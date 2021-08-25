
#include "../include/tcp_server.h"


void TcpServer::subscribe(const server_observer_t & observer) {
    m_subscibers.push_back(observer);
}

void TcpServer::unsubscribeAll() {
    m_subscibers.clear();
}

void TcpServer::printClients() {
    for (uint i=0; i<m_clients.size(); i++) {
        std::string connected = m_clients[i].isConnected() ? "True" : "False";
        std::cout << "-----------------\n" <<
                  "IP address: " << m_clients[i].getIp() << std::endl <<
                  "Connected?: " << connected << std::endl <<
                  "Socket FD: " << m_clients[i].getFileDescriptor() << std::endl <<
                  "Message: " << m_clients[i].getInfoMessage().c_str() << std::endl;
    }
}

/*
 * Receive client packets, and notify user
 */
void TcpServer::receiveTask(/*TcpServer *context*/) {

    Client * client = &m_clients.back();

    while(client->isConnected()) {
        char msg[MAX_PACKET_SIZE];
        int numOfBytesReceived = recv(client->getFileDescriptor(), msg, MAX_PACKET_SIZE, 0);
        std::cout << numOfBytesReceived << "bytes received" << std::endl;
        std::string recv = msg;
        recv = recv.substr(0, numOfBytesReceived);
        //std::cout << recv << std::endl;
        if(numOfBytesReceived < 1) {
            client->setDisconnected();
            if (numOfBytesReceived == 0) { //client closed connection
                client->setErrorMessage("Client closed connection");
                printf("client closed");
            } else {
                client->setErrorMessage("Unknown Error");
            }
            closesocket(client->getFileDescriptor());
            publishClientDisconnected(*client);
            deleteClient(*client);
            break;
        } else {
            publishClientMsg(*client, recv.c_str() , numOfBytesReceived);
        }
    }
}

/*
 * Erase client from clients vector.
 * If client isn't in the vector, return false. Return
 * true if it is.
 */
bool TcpServer::deleteClient(Client & client) {
    int clientIndex = -1;
    for (uint i=0; i<m_clients.size(); i++) {
        if (m_clients[i] == client) {
            clientIndex = i;
            break;
        }
    }
    if (clientIndex > -1) {
        m_clients.erase(m_clients.begin() + clientIndex);
        return true;
    }
    return false;
}

void TcpServer::startListenThread(){
    serverThread = new std::thread(&TcpServer::listenClient,this);
}

void TcpServer::listenClient(){
    while (1) {
        Client client = this->acceptClient(0);
        std::cout << "start client accepted" << std::endl;
        if (client.isConnected()) {
            std::cout << "Got client with IP: " << client.getIp() << std::endl;
            printClients();
        }
        else {
            std::cout << "Accepting client failed: " << client.getInfoMessage() << std::endl;
            break;
        }
        Sleep(1);
    }
}


/*
 * Publish incoming client message to observer.
 * Observers get only messages that originated
 * from clients with IP address identical to
 * the specific observer requested IP
 */
void TcpServer::publishClientMsg(const Client & client, const char * msg, size_t msgSize) {
    for (uint i=0; i<m_subscibers.size(); i++) {
        if (m_subscibers[i].wantedIp == client.getIp() || m_subscibers[i].wantedIp.empty()) {
            if (m_subscibers[i].incoming_packet_func != NULL) {
                (*m_subscibers[i].incoming_packet_func)(client, msg, msgSize);
            }
        }
    }
}

/*
 * Publish client disconnection to observer.
 * Observers get only notify about clients
 * with IP address identical to the specific
 * observer requested IP
 */
void TcpServer::publishClientDisconnected(const Client & client) {
    for (uint i=0; i<m_subscibers.size(); i++) {
        if (m_subscibers[i].wantedIp == client.getIp()) {
            if (m_subscibers[i].disconnected_func != NULL) {
                (*m_subscibers[i].disconnected_func)(client);
            }
        }
    }
}

void TcpServer::addClient(std::string ip, incoming_packet_func_t onIncomingMsg, disconnected_func_t onDisconnection){
    server_observer_t observer;
    observer.wantedIp = ip;
    observer.incoming_packet_func = onIncomingMsg;
    observer.disconnected_func = onDisconnection;
    subscribe(observer);
}

/*
 * Bind port and start listening
 * Return tcp_ret_t
 */
pipe_ret_t TcpServer::start(int port) {
    m_sockfd = 0;
    m_clients.reserve(10);
    m_subscibers.reserve(10);
    pipe_ret_t ret;

    m_sockfd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd == -1) { //socket failed
        char errMsg[256];
        ret.success = false;
        //ret.msg = strerror_s(errMsg, 256, errno);
        ret.msg = "socket fail";

        return ret;
    }

    //linux version?
    /*
    // set socket for reuse (otherwise might have to wait 4 minutes every time socket is closed)
    int option = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    */

    bool bReuseaddr = TRUE;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(bReuseaddr));

    memset(&m_serverAddress, 0, sizeof(m_serverAddress));
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(port);

    int bindSuccess = bind(m_sockfd, (struct sockaddr *)&m_serverAddress, sizeof(m_serverAddress));
    if (bindSuccess == -1) { // bind failed
        char errMsg[256];
        ret.success = false;
        //ret.msg = strerror_s(errMsg, 256, errno);
        ret.msg = "bind fail";
        return ret;
    }
    const int clientsQueueSize = 5;
    int listenSuccess = listen(m_sockfd, clientsQueueSize);
    if (listenSuccess == -1) { // listen failed
        char errMsg[256];
        ret.success = false;
        //ret.msg = strerror_s(errMsg, 256, errno);
        ret.msg = "listen fail";
        return ret;
    }
    ret.success = true;
    return ret;
}

/*
 * Accept and handle new client socket. To handle multiple clients, user must
 * call this function in a loop to enable the acceptance of more than one.
 * If timeout argument equal 0, this function is executed in blocking mode.
 * If timeout argument is > 0 then this function is executed in non-blocking
 * mode (async) and will quit after timeout seconds if no client tried to connect.
 * Return accepted client
 */
Client TcpServer::acceptClient(uint timeout) {
    socklen_t sosize  = sizeof(m_clientAddress);
    Client newClient;

    if (timeout > 0) {
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        FD_ZERO(&m_fds);
        FD_SET(m_sockfd, &m_fds);
        int selectRet = select(m_sockfd + 1, &m_fds, NULL, NULL, &tv);
        if (selectRet == -1) { // select failed
            newClient.setErrorMessage("select failed");
            return newClient;
        } else if (selectRet == 0) { // timeout
            newClient.setErrorMessage("Timeout waiting for client");
            return newClient;
        } else if (!FD_ISSET(m_sockfd, &m_fds)) { // no new client
            newClient.setErrorMessage("File descriptor is not set");
            return newClient;
        }
    }

    int file_descriptor = accept(m_sockfd, (struct sockaddr*)&m_clientAddress, &sosize);
    if (file_descriptor == -1) { // accept failed
        newClient.setErrorMessage("accept fail");
        return newClient;
    }

    newClient.setFileDescriptor(file_descriptor);
    newClient.setConnected();
    
    //deprecated
    //newClient.setIp(inet_ntoa(m_clientAddress.sin_addr));

    char str[INET_ADDRSTRLEN];
    newClient.setIp(inet_ntop(AF_INET, &m_clientAddress.sin_addr, str, INET_ADDRSTRLEN));

    m_clients.push_back(newClient);
    m_clients.back().setThreadHandler(std::bind(&TcpServer::receiveTask, this));

    return newClient;
}

/*
 * Send message to all connected clients.
 * Return true if message was sent successfully to all clients
 */
pipe_ret_t TcpServer::sendToAllClients(const char * msg, size_t size) {
    pipe_ret_t ret;
    for (uint i=0; i<m_clients.size(); i++) {
        ret = sendToClient(m_clients[i], msg, size);
        if (!ret.success) {
            return ret;
        }
    }
    ret.success = true;
    return ret;
}

/*
 * Send message to specific client (determined by client IP address).
 * Return true if message was sent successfully
 */
pipe_ret_t TcpServer::sendToClient(const Client & client, const char * msg, size_t size){
    pipe_ret_t ret;
    int numBytesSent = send(client.getFileDescriptor(), (char *)msg, size, 0);
    if (numBytesSent < 0) { // send failed
        char errMsg[256];
        ret.success = false;
        ret.msg = strerror_s(errMsg, 256, errno);
        return ret;
    }
    if ((uint)numBytesSent < size) { // not all bytes were sent
        ret.success = false;
        char msg[100];
        sprintf_s(msg, "Only %d bytes out of %lu was sent to client", numBytesSent, size);
        ret.msg = msg;
        return ret;
    }
    ret.success = true;
    return ret;
}

/*
 * Close server and clients resources.
 * Return true is success, false otherwise
 */
pipe_ret_t TcpServer::finish() {
    pipe_ret_t ret;
    for (uint i=0; i<m_clients.size(); i++) {
        m_clients[i].setDisconnected();
        std::cout << "Closing client..." << i << std::endl;
        if (closesocket(m_clients[i].getFileDescriptor()) == -1) { // close failed
            char errMsg[256];
            ret.success = false;
            std::cout << "Closing client...failed " << i << std::endl;
            ret.msg = strerror_s(errMsg, 256, errno);
            return ret;
        }
    }
    std::cout << "Closing server self..." << std::endl;
    if (closesocket(m_sockfd) == -1) { // close failed
        char errMsg[256];
        ret.success = false;
        std::cout << "Closing server...failed"<< std::endl;
        ret.msg = strerror_s(errMsg, 256, errno);
        return ret;
    }
    m_clients.clear();
    ret.success = true;
    return ret;
}
