#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <map>
#include <list>
using namespace std;
#include <WS2tcpip.h> 
#pragma comment(lib, "Ws2_32.lib") 
#define MAX_BUFFER 1024
#define SERVER_PORT 3500 

// 체스말 이동 키값
#define KEY_LEFT	'4'
#define KEY_RIGHT	'6'
#define KEY_UP		'8'
#define KEY_DOWN	'2'
#define KEY_NULL	'0'


struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	SOCKET socket;
	char messageBuffer[MAX_BUFFER];
};

struct PLAYER_INFO	// 체스말의 좌표
{
	int pos_x;
	int pos_y;
	int id;
	bool connect;
};

struct KEY		// 체스말의 이동키값을 받아올 구조체
{
	char cKey;
	int id;
};

map <SOCKET, SOCKETINFO> clients;  // 소켓 컨테이너 맵
PLAYER_INFO Arr_PlayersInfo[10];
list<int> idxList;

int idx = 0;


void KeyMessage(const char* key, PLAYER_INFO& p_info);	// 키값에 따라 좌표이동
void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);


int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); //wsaflag를 반드시 넣어야한다.
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, 5);
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);
	SOCKET clientSocket;
	DWORD flags;

	for (int i = 0; i < 10; ++i)
	{
		idxList.push_back(i);
	}

	while (true)
	{
		clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			printf("err  \n");
			break;
		}
		else
		{
			printf("success  \n");
		}

		if (clients.size() < 10)
		{
			clients[clientSocket] = SOCKETINFO{};
			memset(&clients[clientSocket], 0, sizeof(struct SOCKETINFO));
			clients[clientSocket].socket = clientSocket;

			idx = idxList.front();
			idxList.pop_front();

			Arr_PlayersInfo[idx] = PLAYER_INFO{ 0, 0, idx, true };
			Arr_PlayersInfo[idx].id = idx;
			clients[clientSocket].dataBuffer.len = sizeof(PLAYER_INFO);  // 플레이어구조체의 사이즈
			clients[clientSocket].dataBuffer.buf = (char*)&Arr_PlayersInfo[idx];
			flags = 0;
			clients[clientSocket].overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
			WSASend(clients[clientSocket].socket, &clients[clientSocket].dataBuffer, 1, NULL,
				0, &(clients[clientSocket].overlapped), send_callback);// 등록 후 데이터가 오면 recv_callback에서 메시지를 처리한다.
		}
	}

	closesocket(listenSocket);
	WSACleanup();
}


void KeyMessage(const char* key, PLAYER_INFO& PlayerInfo)
{
	if (KEY_RIGHT == *key)
	{
		PlayerInfo.pos_x += 50;
		if (350 < PlayerInfo.pos_x)
		{
			PlayerInfo.pos_x -= 50;
		}
	}
	if (KEY_LEFT == *key)
	{
		PlayerInfo.pos_x -= 50;
		if (0 > PlayerInfo.pos_x)
		{
			PlayerInfo.pos_x += 50;
		}
	}
	if (KEY_UP == *key)
	{
		PlayerInfo.pos_y -= 50;
		if (0 > PlayerInfo.pos_y)
		{
			PlayerInfo.pos_y += 50;
		}
	}
	if (KEY_DOWN == *key)
	{
		PlayerInfo.pos_y += 50;
		if (350 < PlayerInfo.pos_y)
		{
			PlayerInfo.pos_y -= 50;
		}
	}
}

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	KEY keyInfo{};
	memcpy(&keyInfo, clients[client_s].messageBuffer, sizeof(KEY)); // 받은 키 구조체 데이터 복사

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		Arr_PlayersInfo[keyInfo.id] = PLAYER_INFO{};
		idxList.push_front(keyInfo.id);
		return;
	}
	// 클라이언트가 closesocket을 했을 경우 

	clients[client_s].messageBuffer[dataBytes] = 0;

	KeyMessage(&keyInfo.cKey, Arr_PlayersInfo[keyInfo.id]); // 키 입력에 따라 플레이어구조체 좌표 이동

	// cout << "From client : " << clients[client_s].messageBuffer << " (" << dataBytes << ") bytes)\n";

	clients[client_s].dataBuffer.len = sizeof(Arr_PlayersInfo); // 버퍼사이즈
	clients[client_s].dataBuffer.buf = (char*)&Arr_PlayersInfo; // 데이터 버퍼


	memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;
	WSASend(client_s, &(clients[client_s].dataBuffer), 1, &dataBytes, 0,
		&(clients[client_s].overlapped), send_callback);
}

void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD receiveBytes = 0;
	DWORD flags = 0;
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);
	if (dataBytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s); return;
	}		// 클라이언트가 closesocket을 했을 경우

	clients[client_s].dataBuffer.len = MAX_BUFFER;
	clients[client_s].dataBuffer.buf = clients[client_s].messageBuffer;
	// cout << "Sent : " << clients[client_s].messageBuffer << " (" << dataBytes << " bytes)\n";
	memset(&(clients[client_s].overlapped), 0x00, sizeof(WSAOVERLAPPED));
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;
	WSARecv(client_s, &clients[client_s].dataBuffer, 1, 0, &flags,
		&(clients[client_s].overlapped), recv_callback);
}