// SelectServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"   
#include <winsock.h>   
#include <stdio.h>   
#define PORT  5150   
#define MSGSIZE  1024*4   
#pragma comment(lib, "ws2_32.lib")   
int g_iTotalConn = 0; //socket数组元素个数  全局
SOCKET g_CliSocketArr[FD_SETSIZE];
DWORD WINAPI WorkerThread(LPVOID lpParam);

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;   
	SOCKET sListen, sClient;   
	SOCKADDR_IN local, client;   
	int iAddrSize = sizeof(SOCKADDR_IN);   
	DWORD dwThreadId;   
	// Initialize windows socket library   
	WSAStartup(0x0202, &wsaData);   
	// Create listening socket   
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
	// Bind   
	local.sin_family = AF_INET;   
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);   
	local.sin_port = htons(PORT);   
	bind(sListen, (sockaddr*)&local, sizeof(SOCKADDR_IN));   
	// Listen   
	listen(sListen, 3);   
	// Create worker thread   
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);//开启工作者线程   
	while (TRUE)    
	{   
		// Accept a connection   
		sClient = accept(sListen, (sockaddr*)&client, &iAddrSize);   
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));   
		// Add socket to g_CliSocketArr   
		g_CliSocketArr[g_iTotalConn++] = sClient;   //把所有接入的客户端socket加进socket数组
	}   
	return 0;   
}   

DWORD WINAPI WorkerThread(LPVOID lpParam)   
{   
	int i;   
	fd_set fdread;   //可以理解为一个集合，这个集合中存放的是文件描述符
	fd_set fdwrite;
	fd_set fdexcept;
	int ret;   
	struct timeval tv = {1, 0}; //paramer1:S paramer2:ms  
	char szMessage[MSGSIZE];   
	memset(szMessage, 0, MSGSIZE);
	while (TRUE)    
	{
		ret = 0;
		memset(szMessage, 0, MSGSIZE);
		if (g_iTotalConn<=0)//当接入客户端为0时，select返回值为-1，代表出错
		{
			printf("0 client is join!\n");
			Sleep(1000);
			continue;
		}

		FD_ZERO(&fdread);   
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcept);
		for (i = 0; i < g_iTotalConn; i++)    
		{   
			FD_SET(g_CliSocketArr[i], &fdread);   
			FD_SET(g_CliSocketArr[i], &fdwrite); 
			FD_SET(g_CliSocketArr[i], &fdexcept); 
		}  //用客户端socket数组初始化 可以读的客户端socket数组

		printf("there are %d sockets in fdset！\n", fdread.fd_count);

		// We only care read event
		//select 第一个参数在windows下可以忽略
		ret = select(0, &fdread, &fdwrite, &fdexcept, &tv); //检测客户端socket是否可读 （可写，异常） 
		if (ret == 0)    //ret 代表满足条件的socket数目
		{   
			printf("%d sockets is ready to send message!\n", ret);
			// Time expired   
			continue;   
		}   
		
		//处理读、写、异常
		for (i = 0; i < g_iTotalConn; i++)    
		{   
			if (FD_ISSET(g_CliSocketArr[i], &fdread))   //测试当前客户端i是否 可读
			{   
				printf("client:%d is ready to send some messages to server\n", g_CliSocketArr[i]);
				// A read event happened on g_CliSocketArr   
				ret = recv(g_CliSocketArr[i], szMessage, MSGSIZE, 0);   
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))    
				{   
					// Client socket closed   
					printf("Client socket %d closed.\n", g_CliSocketArr[i]);   
					closesocket(g_CliSocketArr[i]);   

// 				if (i < g_iTotalConn-1)    
// 				{   
						g_CliSocketArr[i--] = g_CliSocketArr[--g_iTotalConn];  
					//FD_CLR(g_CliSocketArr[i], &fdread);
//					}
				}    
			}
			
			if (FD_ISSET(g_CliSocketArr[i], &fdwrite))   //测试当前客户端i是否 可写
			{
				printf("client:%d is ready to receive message from server\n", g_CliSocketArr[i]); 
				// We reveived a message from client   
				szMessage[ret] = '\0';   
				send(g_CliSocketArr[i], szMessage, strlen(szMessage), 0);	
			}
			
			if (FD_ISSET(g_CliSocketArr[i], &fdexcept)) //测试当前客户端i是否 异常
			{
				printf("client:%d has some excepts\n", g_CliSocketArr[i]); 
			}
		}
		Sleep(1000);
	}   
	return 0;
}

