

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

    // configure and register observer
    client_observer_t observer;
	observer.wantedIp = "127.0.0.1";
	observer.incoming_packet_func = onIncomingMsg;
	observer.disconnected_func = onDisconnection;
	client.subscribe(observer);

	// connect client to an open server
    pipe_ret_t connectRet = client.connectTo("127.0.0.1", 65123);
	if (connectRet.success) {
		std::cout << "Client connected successfully" << std::endl;
	} else {
		std::cout << "Client failed to connect: " << connectRet.msg << std::endl;
		return EXIT_FAILURE;
	}

	// send messages to server
	while(1)
	{
		pipe_ret_t sendRet;
		std::string msg;
		for (int i = 0; i < 1000; i++) {

			msg = "hello server" + std::to_string(i) + "\n";
			sendRet = client.sendMsg(msg, msg.size());
			if (!sendRet.success) {
				std::cout << "Failed to send msg: " << sendRet.msg << std::endl;
				break;
			}
			system("pause");
		}

		msg = "11234\n";
		sendRet = client.sendMsg(msg, msg.size());
		if (!sendRet.success) {
			std::cout << "Failed to send msg: " << sendRet.msg << std::endl;
			break;
		}

		msg = "print\n";
		sendRet = client.sendMsg(msg, msg.size());
		if (!sendRet.success) {
			std::cout << "Failed to send msg: " << sendRet.msg << std::endl;
			break;
		}

		msg = "quit\n";
		sendRet = client.sendMsg(msg, msg.size());
		if (!sendRet.success) {
			std::cout << "Failed to send msg: " << sendRet.msg << std::endl;
			break;
		}
		Sleep(100000);
	}

	return 0;
}
