#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <WS2tcpip.h>
#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <conio.h>
using namespace std;
#pragma comment (lib,"WS2_32.LIB")

constexpr int BUF_SIZE = 1024;
constexpr short PORT = 3500;
constexpr char SERVER_ADDR[] = "127.0.0.1";
//constexpr char SERVER_ADDR[] = "192.168.0.10";   // 나의 IP 주소



int Window_Size_X = 500;
int Window_Size_Y = 500;

HINSTANCE g_hInst;
LPCTSTR lpszClass = "Window Class Name";
LPCTSTR lpszWindowName = "Game Server - 1";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&WndClass);

    hWnd = CreateWindow
    (lpszClass,
        lpszWindowName,
        (WS_OVERLAPPEDWINDOW),
        0,
        0,
        Window_Size_X,
        Window_Size_Y,
        NULL,
        (HMENU)NULL,
        hInstance,
        NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    while (GetMessage(&Message, 0, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return Message.wParam;
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    COLORREF text_color;
    COLORREF bk_color;
    HBRUSH hBrush, oldBrush;

    static RECT Pan[8][8];

    static WSADATA WSAData;
    static SOCKET s_socket;
    static SOCKADDR_IN server_a;
    memset(&server_a, 0, sizeof(server_a));

    switch (iMessage)
    {
    case WM_CREATE:
        WSAStartup(MAKEWORD(2, 0), &WSAData);
        s_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0); // 소켓 만들기

        server_a.sin_family = AF_INET;
        server_a.sin_port = htons(PORT);  // host to netwrok 앤디안을 맞춰서
        inet_pton(AF_INET, SERVER_ADDR, &server_a.sin_addr);
        WSAConnect(s_socket, (sockaddr*)&server_a, sizeof(server_a), NULL, NULL, NULL, NULL);
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_LEFT:
            player.test = 1;
            send(s_socket, (char*)&player, sizeof(Player), 0);
            break;
        case VK_RIGHT:
            player.test = 2;
            send(s_socket, (char*)&player, sizeof(Player), 0);
            break;
        case VK_DOWN:
            player.test = 3;
            send(s_socket, (char*)&player, sizeof(Player), 0);
            break;
        case VK_UP:
            player.test = 4;
            send(s_socket, (char*)&player, sizeof(Player), 0);
            break;
        }
        recv(s_socket, (char*)&player, sizeof(Player), 0);
        InvalidateRect(hWnd, NULL, TRUE); // FALSE로 하면 이어짐
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Pan[i][j].left = 0 + i * 50;
                Pan[i][j].right = 50 + i * 50;
                Pan[i][j].top = 0 + j * 50;
                Pan[i][j].bottom = 50 + j * 50;
                if (i % 2 == 1 && j % 2 == 0) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

                    Rectangle(hdc, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(hBrush);
                }

                else if (i % 2 == 0 && j % 2 == 1) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

                    Rectangle(hdc, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(hBrush);
                }

                else {
                    Rectangle(hdc, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                }
            }
        }

        hBrush = CreateSolidBrush(RGB(255, 0, 0));
        oldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        Ellipse(hdc, player.width, player.height, player.width + 50, player.height + 50);
        SelectObject(hdc, oldBrush);
        DeleteObject(hBrush);


        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        closesocket(s_socket);
        WSACleanup();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}