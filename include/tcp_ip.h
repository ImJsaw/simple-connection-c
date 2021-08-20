#pragma once

#pragma comment(lib, "Ws2_32.lib")

#include <vector>
#include <stdio.h>
#include <stdlib.h>

//#include <unistd.h>
#include <io.h> 
#include <process.h> 

#include <sys/types.h>

/*
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/
#include <winsock2.h>
#include <windows.h>

#include <thread>
#include <functional>
#include <cstring>
#include <errno.h>
#include <string.h>

#include <iostream>
#include "pipe_ret_t.h"

#include <ws2tcpip.h>

#define uint unsigned int