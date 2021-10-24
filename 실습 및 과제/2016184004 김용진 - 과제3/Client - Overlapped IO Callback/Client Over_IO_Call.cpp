#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
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

#define MAX_BUFFER 1024
#define SERVER_PORT 3500
#define SERVER_IP "127.0.0.1"

#define KEY_NULL '0'
#define KEY_DOWN '2'
#define KEY_LEFT '4'
#define KEY_RIGHT '6'
#define KEY_UP '8'

int Window_Size_X = 500;
int Window_Size_Y = 500;

HINSTANCE g_hInst;
LPCTSTR lpszClass = "Window Class Name";
LPCTSTR lpszWindowName = "Game Server - 3";
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

struct PLAYER_INFO {
    int pos_x;
    int pos_y;
    int id;
    bool connect;
};

struct KEY {
    char cKey;
    int id;
};

int r[10];
int g[10];
int b[10];

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hdc, mdc;
    static HDC BackBuffer;
    static HBITMAP hBitmap, BackBit, BackoBit;
    PAINTSTRUCT ps;
    COLORREF text_color;
    COLORREF bk_color;
    HBRUSH hBrush, oldBrush;
    RECT rcClient;
    static RECT Pan[8][8];

    ////////////////////////////

    // 체스말 좌표
    static int x, y;
    // 체스말 활성화
    static bool chessRect = false;
    // 소켓 연결 활성화
    static bool SockConnect = false;

    // Server / Client
    static WSADATA WSAData;
    static SOCKET serverSocket;
    static sockaddr_in serverAddr;

    static PLAYER_INFO PlayersInfo[10]; // 체스말 정보 구조체 배열
    static KEY keyInfo{ KEY_NULL };    // 입력된 키 정보 구조체
    static char messageBuffer[MAX_BUFFER];


    switch (iMessage)
    {
    case WM_CREATE:

        WSAStartup(MAKEWORD(2, 0), &WSAData);   //  네트워크 기능을 사용하기 위함, 인터넷 표준을 사용하기 위해
        serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
        memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);// ipv4에서 ipv6로 변환
        connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        recv(serverSocket, messageBuffer, sizeof(PLAYER_INFO), 0);
        PlayersInfo[0] = *(PLAYER_INFO*)messageBuffer;
        keyInfo.id = PlayersInfo[0].id;

        SockConnect = true;
        chessRect = true;

        for (int i = 0; i < 10; ++i) {
            r[i] = rand() % 25 * 10;
            g[i] = rand() % 25 * 10;
            b[i] = rand() % 25 * 10;
        }

        // Timer
        SetTimer(hWnd, 1, 50, NULL);

        break;

    case WM_TIMER:
        switch (wParam)
        {
        case 1:
            if (true == SockConnect)
            {
                send(serverSocket, (char*)&keyInfo, sizeof(KEY), 0);
                recv(serverSocket, (char*)&PlayersInfo, sizeof(PlayersInfo), 0);
                keyInfo.cKey = KEY_NULL;
            }
            break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_PAINT:

        // Server Client
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rcClient);

        BackBuffer = CreateCompatibleDC(hdc);

        BackoBit = CreateBitmap(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, 1, 32, NULL);
        hBitmap = (HBITMAP)SelectObject(BackBuffer, BackoBit);


        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Pan[i][j].left = 0 + i * 50;
                Pan[i][j].right = 50 + i * 50;
                Pan[i][j].top = 0 + j * 50;
                Pan[i][j].bottom = 50 + j * 50;
                if (i % 2 == 1 && j % 2 == 0) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

                    Rectangle(BackBuffer, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                    SelectObject(BackBuffer, oldBrush);
                    DeleteObject(hBrush);
                }

                else if (i % 2 == 0 && j % 2 == 1) {
                    hBrush = CreateSolidBrush(RGB(0, 0, 0));
                    oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);

                    Rectangle(BackBuffer, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                    SelectObject(BackBuffer, oldBrush);
                    DeleteObject(hBrush);
                }

                else {
                    Rectangle(BackBuffer, Pan[i][j].left, Pan[i][j].top, Pan[i][j].right, Pan[i][j].bottom);
                }
            }
        }

        // 체스말 Rect
        if (true == chessRect)
        {
            for (int i = 0; i < 10; ++i)
            {
                if (true == PlayersInfo[i].connect)
                {
                    if (keyInfo.id == i)   // 자기자신 체스 말
                    {
                        hBrush = CreateSolidBrush(RGB(255, 0, 0));
                        oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
                        Ellipse(BackBuffer, PlayersInfo[i].pos_x, PlayersInfo[i].pos_y, PlayersInfo[i].pos_x + 50, PlayersInfo[i].pos_y + 50);
                        SelectObject(BackBuffer, oldBrush);
                        DeleteObject(hBrush);

                        TCHAR str[10];
                        wsprintf(str, TEXT("ME"));
                        TextOut(BackBuffer, PlayersInfo[i].pos_x + 10, PlayersInfo[i].pos_y + 10, str, lstrlen(str));
                    }
                    else
                    {
                        // 다른 클라이언트 체스 말


                        hBrush = CreateSolidBrush(RGB(r[i], g[i], b[i]));
                        oldBrush = (HBRUSH)SelectObject(BackBuffer, hBrush);
                        Ellipse(BackBuffer, PlayersInfo[i].pos_x, PlayersInfo[i].pos_y, PlayersInfo[i].pos_x + 50, PlayersInfo[i].pos_y + 50);
                        SelectObject(BackBuffer, oldBrush);
                        DeleteObject(hBrush);

                        TCHAR str[10];
                        wsprintf(str, TEXT("%dPlayer"), i + 1);
                        TextOut(BackBuffer, PlayersInfo[i].pos_x + 10, PlayersInfo[i].pos_y + 10, str, lstrlen(str));
                    }

                }
            }
        }

        BitBlt(hdc, 0, 0, 400, 400, BackBuffer, 0, 0, SRCCOPY);
        DeleteDC(BackBuffer);
        DeleteObject(hBitmap);
        EndPaint(hWnd, &ps);
        break;
    case WM_KEYUP:
        keyInfo.cKey = KEY_NULL;
        break;

    case WM_KEYFIRST:
        if (wParam == VK_RIGHT)
        {
            keyInfo.cKey = KEY_RIGHT;
        }
        else if (wParam == VK_LEFT)
        {
            keyInfo.cKey = KEY_LEFT;
        }
        else if (wParam == VK_UP)
        {
            keyInfo.cKey = KEY_UP;
        }
        else if (wParam == VK_DOWN)
        {
            keyInfo.cKey = KEY_DOWN;
        }

        InvalidateRect(hWnd, NULL, FALSE); // FALSE로 하면 이어짐  
        break;

    case WM_DESTROY:
        SelectObject(BackBuffer, BackoBit);
        DeleteObject(BackBit);
        DeleteDC(BackBuffer);
        // closesocket(s_socket);
        // WSACleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
    return 0;
}