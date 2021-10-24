#include <iostream>
#include <WS2tcpip.h>
using namespace std;
#pragma comment (lib,"WS2_32.LIB")
constexpr int BUF_SIZE = 1024;
constexpr short PORT = 3500;

void err_disp(const char* msg, int err_no)
{
	WCHAR* h_mess;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&h_mess, 0, NULL);
	cout << msg;
	wcout << L"  에러 => " << h_mess << endl;
	while (true);
	LocalFree(h_mess);
}

#pragma pack(1)
typedef struct Player
{
	int width;
	int height;
	int test;
} Player;
#pragma pack()

Player player;


int main()
{
	// int len = sizeof(player);

	wcout.imbue(std::locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	SOCKET s_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0); // 소켓 만들기
	SOCKADDR_IN server_a; // 서버 주소 선언
	memset(&server_a, 0, sizeof(server_a)); // 서버 주소 초기화 반드시 필요
	// 서버 주소에 값 하나하나 넣어 줘야함
	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);  // host to netwrok 앤디안을 맞춰서
	server_a.sin_addr.s_addr = INADDR_ANY; // 모든 주소를 받기 위해

	::bind(s_socket, (sockaddr*)&server_a, sizeof(server_a)); // 소켓,소켓 주소,소켓 주소의 크기
	listen(s_socket, SOMAXCONN);


	while (true) {
		SOCKADDR_IN client_a;
		INT a_size = sizeof(client_a);
		// 오류 검출 해보기 위해 a_size = 0;
		SOCKET c_socket = WSAAccept(s_socket, (sockaddr*)&client_a, &a_size, NULL, NULL); // Accept가 됐으면 클라이언트 소켓이 필요
		if (SOCKET_ERROR == c_socket) {
			err_disp("WSAAccept", WSAGetLastError());
		}
		cout << "New client accepted.\n";
		//cout << "IP Address : " << (sockaddr*)&client_a << endl;
		//cout << "IP Address : " << (sockaddr*)&server_a << endl;


		while (true) {

			recv(c_socket, (char*)&player, sizeof(Player), 0);

			if (player.test == 1) {
				player.width -= 50;
				if (player.width < 0)
					player.width = 0;
			}

			if (player.test == 2) {
				player.width += 50;
				if (player.width > 350)
					player.width = 350;
			}

			if (player.test == 3) {
				player.height += 50;
				if (player.height > 350)
					player.height = 350;
			}
			if (player.test == 4) {
				player.height -= 50;
				if (player.height < 0)
					player.height = 0;
			}

			send(c_socket, (char*)&player, sizeof(Player), 0);
		}
		cout << "Client connection closed.\n";
	}
	closesocket(s_socket);
	WSACleanup();
}