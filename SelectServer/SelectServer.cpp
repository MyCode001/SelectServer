// SelectServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"   
#include <winsock.h>   
#include <stdio.h>   
#define PORT  5150   
#define MSGSIZE  1024*4   
#pragma comment(lib, "ws2_32.lib")   
int g_iTotalConn = 0; //socket����Ԫ�ظ���  ȫ��
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
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);//�����������߳�   
	while (TRUE)    
	{   
		// Accept a connection   
		sClient = accept(sListen, (sockaddr*)&client, &iAddrSize);   
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));   
		// Add socket to g_CliSocketArr   
		g_CliSocketArr[g_iTotalConn++] = sClient;   //�����н���Ŀͻ���socket�ӽ�socket����
	}   
	return 0;   
}   

DWORD WINAPI WorkerThread(LPVOID lpParam)   
{   
	int i;   
	fd_set fdread;   //�������Ϊһ�����ϣ���������д�ŵ����ļ�������
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
		if (g_iTotalConn<=0)//������ͻ���Ϊ0ʱ��select����ֵΪ-1���������
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
		}  //�ÿͻ���socket�����ʼ�� ���Զ��Ŀͻ���socket����

		printf("there are %d sockets in fdset��\n", fdread.fd_count);

		// We only care read event
		//select ��һ��������windows�¿��Ժ���
		ret = select(0, &fdread, &fdwrite, &fdexcept, &tv); //���ͻ���socket�Ƿ�ɶ� ����д���쳣�� 
		if (ret == 0)    //ret ��������������socket��Ŀ
		{   
			printf("%d sockets is ready to send message!\n", ret);
			// Time expired   
			continue;   
		}   
		
		//�������д���쳣
		for (i = 0; i < g_iTotalConn; i++)    
		{   
			if (FD_ISSET(g_CliSocketArr[i], &fdread))   //���Ե�ǰ�ͻ���i�Ƿ� �ɶ�
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
			
			if (FD_ISSET(g_CliSocketArr[i], &fdwrite))   //���Ե�ǰ�ͻ���i�Ƿ� ��д
			{
				printf("client:%d is ready to receive message from server\n", g_CliSocketArr[i]); 
				// We reveived a message from client   
				szMessage[ret] = '\0';   
				send(g_CliSocketArr[i], szMessage, strlen(szMessage), 0);	
			}
			
			if (FD_ISSET(g_CliSocketArr[i], &fdexcept)) //���Ե�ǰ�ͻ���i�Ƿ� �쳣
			{
				printf("client:%d has some excepts\n", g_CliSocketArr[i]); 
			}
		}
		Sleep(1000);
	}   
	return 0;
}

