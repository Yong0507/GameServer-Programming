#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>

int Window_Size_X = 500;
int Window_Size_Y = 500;

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Game Server - 1";
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    COLORREF text_color;
    COLORREF bk_color;
    HBRUSH hBrush, oldBrush;

    static RECT Pan[8][8];
    static int width = 0;
    static int height = 0;

    switch (iMessage)
    {
        case WM_CREATE:
            break;

        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_LEFT:
                    width -= 50;
                    if (width < 0)
                        width = 0;
                    break;
                case VK_RIGHT:
                    width += 50;
                    if (width > 350)
                        width = 350;
                    break;
                case VK_DOWN:
                    height += 50;
                    if (height > 350)
                        height = 350;
                    break;
                case VK_UP:
                    height -= 50;
                    if (height < 0)
                        height = 0;
                    break;
            }
            
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

            Ellipse(hdc,width, height, width+50, height+50);
            SelectObject(hdc, oldBrush);
            DeleteObject(hBrush);


        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}