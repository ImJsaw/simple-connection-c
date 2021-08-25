
#include "../include/tcp_client.h"
#define uint unsigned int 

pipe_ret_t TcpClient::connectTo(const std::string & address, int port) {
    m_sockfd = 0;
    pipe_ret_t ret;

    m_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (m_sockfd == -1) { //socket failed
        char errMsg[256];
        ret.success = false;
        ret.msg = strerror_s(errMsg, 256, errno);
        return ret;
    }

    //only linux
    //int inetSuccess = inet_aton(address.c_str(), &m_server.sin_addr);

    int inetSuccess = inet_pton(AF_INET, address.c_str(), &m_server.sin_addr);


    if(!inetSuccess) { // inet_addr failed to parse address
        ret.success = false;
        ret.msg = "Failed to resolve hostname";
        return ret;
    }
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons( port );

    int connectRet = connect(m_sockfd , (struct sockaddr *)&m_server , sizeof(m_server));
    if (connectRet == -1) {
        char errMsg[256];
        ret.success = false;
        ret.msg = strerror_s(errMsg, 256, errno);
        return ret;
    }
    m_receiveTask = new std::thread(&TcpClient::ReceiveTask, this);
    ret.success = true;
    return ret;
}

bool TcpClient::connectServer(int port){
    pipe_ret_t result = connectTo(server_ip, port);
    if (!result.success) {
        cout << "Failed to send msg: " << result.msg << endl;
    }
    return result.success;
}

bool TcpClient::sendToServer(string msg){
    pipe_ret_t result = sendMsg(msg, msg.size());
    if (!result.success) {
        cout << "Failed to send msg: " << result.msg << endl;
    }
    return result.success;
}


pipe_ret_t TcpClient::sendMsg(std::string msg, size_t size) {
    pipe_ret_t ret;
    int numBytesSent = send(m_sockfd, msg.c_str(), size, 0);
    if (numBytesSent < 0 ) { // send failed
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

void TcpClient::subscribe(const client_observer_t & observer) {
    m_subscibers.push_back(observer);
}

void TcpClient::unsubscribeAll() {
    m_subscibers.clear();
}

/*
 * Publish incoming client message to observer.
 * Observers get only messages that originated
 * from clients with IP address identical to
 * the specific observer requested IP
 */
void TcpClient::publishServerMsg(const char * msg, size_t msgSize) {
    for (uint i=0; i<m_subscibers.size(); i++) {
        if (m_subscibers[i].incoming_packet_func != NULL) {
            (*m_subscibers[i].incoming_packet_func)(msg, msgSize);
        }
    }
}

/*
 * Publish client disconnection to observer.
 * Observers get only notify about clients
 * with IP address identical to the specific
 * observer requested IP
 */
void TcpClient::publishServerDisconnected(const pipe_ret_t & ret) {
    for (uint i=0; i<m_subscibers.size(); i++) {
        if (m_subscibers[i].disconnected_func != NULL) {
            (*m_subscibers[i].disconnected_func)(ret);
        }
    }
}

/*
 * Receive server packets, and notify user
 */
void TcpClient::ReceiveTask() {

    while(!stop) {
        char msg[MAX_PACKET_SIZE];
        int numOfBytesReceived = recv(m_sockfd, msg, MAX_PACKET_SIZE, 0);
        if(numOfBytesReceived < 1) {
            pipe_ret_t ret;
            ret.success = false;
            stop = true;
            if (numOfBytesReceived == 0) { //server closed connection
                ret.msg = "Server closed connection";
            } else {
                char errMsg[256];
                ret.msg = strerror_s(errMsg, 256, errno);
            }
            publishServerDisconnected(ret);
            finish();
            break;
        } else {
            publishServerMsg(msg, numOfBytesReceived);
        }
    }
}

pipe_ret_t TcpClient::finish(){
    stop = true;
    terminateReceiveThread();
    pipe_ret_t ret;
    if (closesocket(m_sockfd) == -1) { // close failed
        char errMsg[256];
        ret.success = false;
        ret.msg = strerror_s(errMsg, 256, errno);
        return ret;
    }
    ret.success = true;
    return ret;
}

void TcpClient::terminateReceiveThread() {
    if (m_receiveTask != nullptr) {
        m_receiveTask->detach();
        delete m_receiveTask;
        m_receiveTask = nullptr;
    }
}

TcpClient::TcpClient(string ip, incoming_packet_func_t onIncomingMsg, disconnected_func_t onDisconnection){
    // configure and register observer
    server_ip = ip;
    client_observer_t observer;
    observer.wantedIp = server_ip;
    observer.incoming_packet_func = onIncomingMsg;
    observer.disconnected_func = onDisconnection;
    subscribe(observer);
}

TcpClient::TcpClient()
{
}

TcpClient::~TcpClient() {
    terminateReceiveThread();
}
