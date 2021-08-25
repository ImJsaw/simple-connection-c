

#include <iostream>
#include <signal.h>
#include "include/tcp_client.h"

TcpClient client;

// on sig_exit, close client
void sig_exit(int s)
{
	std::cout << "Closing client..." << std::endl;
	pipe_ret_t finishRet = client.finish();
	if (finishRet.success) {
		std::cout << "Client closed." << std::endl;
	} else {
		std::cout << "Failed to close client." << std::endl;
	}
	exit(0);
}

// observer callback. will be called for every new message received by the server
void onIncomingMsg(const char * msg, size_t size) {
	std::string recv = msg;
	recv = recv.substr(0, size);
	std::cout << "Got msg from server: " << recv << std::endl;
}

// observer callback. will be called when server disconnects
void onDisconnection(const pipe_ret_t & ret) {
	std::cout << "Server disconnected: " << ret.msg << std::endl;
	std::cout << "Closing client..." << std::endl;
    pipe_ret_t finishRet = client.finish();
	if (finishRet.success) {
		std::cout << "Client closed." << std::endl;
	} else {
		std::cout << "Failed to close client: " << finishRet.msg << std::endl;
	}
}


int main() {
	std::cout << "Client start" << std::endl;

	int iResult;
	WSADATA wsaData;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}


    //register to SIGINT to close client when user press ctrl+c
	signal(SIGINT, sig_exit);

	client = TcpClient("127.0.0.1", onIncomingMsg, onDisconnection);

	// connect client to an open server
    bool res = client.connectServer(65123);

	// send messages to server
	while(1)
	{
		bool sendRet;
		std::string msg;
		for (int i = 0; i < 3; i++) {

			msg = "hello server" + std::to_string(i);
			for (int l = 0; l < 11;l++) {
				std::string tmp = msg + msg;
				msg = tmp;
			}
			//std::cout << "??send" << msg << std::endl;
			sendRet = client.sendToServer(msg);
		}

		msg = "11234\n";
		sendRet = client.sendToServer(msg);

		msg = "print\n";
		sendRet = client.sendToServer(msg);

		msg = "quit\n";
		sendRet = client.sendToServer(msg);

		Sleep(100000);
	}

	return 0;
}
