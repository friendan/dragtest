#pragma once
#include "SlaveWindow.h"
#include "AppConstants.h"
#include "DrawUtil.h"
#include "AppUtil.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <thread>
#include <mutex>

namespace SlaveWindow
{
    const wchar_t* g_szClassName = L"HexViewWindowClass";
    const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
    const int WM_LOAD_FILE_COMPLETE = WM_USER + 2;
    HWND gMainWindow = NULL;
    HWND gStatusBar = NULL;  // 状态栏句柄
    const int STATUS_BAR_PARTS = 3; // 状态栏列数
    int gStatusBarWidths[STATUS_BAR_PARTS]; // 存储3列宽度
    std::string gHexString = "0123456789ABCDEF";
    std::string gFileName = "test.data";
    std::string gFileNameHexStr;
    std::mutex gDataMutex;

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL RegisterWindowClass(HINSTANCE hInstance);
    void CreateStatusBar(HWND hwnd); // 创建状态栏
    
    BOOL RegisterWindowClass(HINSTANCE hInstance) {
        WNDCLASSEX wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = g_szClassName;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        return RegisterClassEx(&wc) != 0;
    }

    void LoadFileInBackground(const std::string& filePathUtf8, const std::string& fileNameUtf8) {
        std::string hexData = AppUtil::FileToHexString(filePathUtf8);
        std::lock_guard<std::mutex> lock(gDataMutex);
        gHexString = hexData;
        gFileName = fileNameUtf8;
        gFileNameHexStr = AppUtil::StringToHexString(gFileName);
        PostMessage(gMainWindow, WM_LOAD_FILE_COMPLETE, 0, 0);
    }

    void StartMainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
        if (!RegisterWindowClass(hInstance)) {
            MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }
        gFileNameHexStr = AppUtil::StringToHexString(gFileName);

        gMainWindow = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            g_szClassName,
            L"GridMap",
            WS_OVERLAPPEDWINDOW | WS_VSCROLL, // 加垂直滚动条，方便查看多内容
            CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
            NULL, NULL, hInstance, NULL
        );

        if (!gMainWindow) {
            MessageBox(NULL, L"窗口创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        // 创建状态栏（窗口创建后立即创建）
        CreateStatusBar(gMainWindow);
        DragAcceptFiles(gMainWindow, TRUE); // 启用拖拽
        DrawUtil::InitDraw();
        
        ShowWindow(gMainWindow, nCmdShow);
        UpdateWindow(gMainWindow);

        // 消息循环
        MSG msg = {0};
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        DrawUtil::UnInitDraw();
    }

    // 创建3列状态栏
    void CreateStatusBar(HWND hwnd) {
        // 创建状态栏控件（WS_CHILD | WS_VISIBLE 必须）
        gStatusBar = CreateWindowEx(
            0,
            STATUSCLASSNAME, // 系统预定义的状态栏类名
            NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, // SBARS_SIZEGRIP：右下角大小调整柄
            0, 0, 0, 0, // 位置大小由WM_SIZE调整
            hwnd,
            (HMENU)1001, // 状态栏控件ID（自定义，唯一即可）
            GetModuleHandle(NULL),
            NULL
        );

        if (!gStatusBar) {
            MessageBox(hwnd, L"状态栏创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        // 初始化列宽（后续WM_SIZE会动态调整）
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int clientWidth = rcClient.right - rcClient.left;
        
        // 3列宽度分配：第1列占30%，第2列占20%，第3列占剩余50%（-1表示到窗口右边界）
        gStatusBarWidths[0] = clientWidth * 0.3;
        gStatusBarWidths[1] = clientWidth * 0.5;
        gStatusBarWidths[2] = -1;

        // 设置状态栏列数和宽度
        SendMessage(gStatusBar, SB_SETPARTS, STATUS_BAR_PARTS, (LPARAM)gStatusBarWidths);
    }

    void RefreshWindow(HWND hwnd){
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
            case WM_CREATE:
                break;
            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                wchar_t szFilePath[MAX_PATH] = {0};
                if (DragQueryFile(hDrop, 0, szFilePath, MAX_PATH)) {
                    const wchar_t* szFileName = PathFindFileNameW(szFilePath);
                    std::string filePathUtf8 = AppUtil::Utf16ToUtf8(szFilePath);
                    std::string fileNameUtf8 = AppUtil::Utf16ToUtf8(szFileName);
                    std::thread fileThread(LoadFileInBackground, filePathUtf8, fileNameUtf8);
                    fileThread.detach(); 
                    SendMessageW(gStatusBar, SB_SETTEXT, 2, (LPARAM)L"ready...");
                }
                DragFinish(hDrop);
                break;
            }
            case WM_LOAD_FILE_COMPLETE: {
                std::lock_guard<std::mutex> lock(gDataMutex);
                DrawUtil::ReStart();
                RefreshWindow(hwnd);
                break;
            }
            case WM_KEYDOWN: {
                if (wParam == VK_SPACE) { // VK_SPACE VK_RETURN VK_ESCAPE VK_F2 VK_TAB
                    DrawUtil::NextPage();
                    RefreshWindow(hwnd);
                }
                else if(wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT){
                    // VK_OEM_MINUS 主键盘减号
                    // VK_SUBTRACT  小键盘减号
                    if(DrawUtil::DecGridSize(1)){
                        RefreshWindow(hwnd);
                    }
                }
                else if(wParam == VK_ADD){
                    // 小键盘加号
                    if(DrawUtil::AddGridSize(1)){
                        RefreshWindow(hwnd);
                    }
                }
                else if(wParam == VK_OEM_PLUS){ // && (GetKeyState(VK_SHIFT) & 0x8000)
                    // 主键盘加号（等号键）
                    if(DrawUtil::AddGridSize(1)){
                        RefreshWindow(hwnd);
                    }
                }
                else if(wParam == VK_ESCAPE){
                    DrawUtil::ReStart();
                    RefreshWindow(hwnd);
                }
                break;
            }
            
            // 窗口大小变化时，调整状态栏宽度和列宽
            case WM_SIZE: {
                if (gStatusBar == NULL) break;

                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                int clientWidth = rcClient.right - rcClient.left;
                int clientHeight = rcClient.bottom - rcClient.top;

                // 调整状态栏大小：宽度=窗口客户区宽度，高度=默认，位置在底部
                MoveWindow(
                    gStatusBar,
                    0, clientHeight - 24, // 24是状态栏默认高度
                    clientWidth, 24,
                    TRUE
                );

                // 重新计算3列宽度（自适应窗口大小）
                gStatusBarWidths[0] = clientWidth * 0.3;
                gStatusBarWidths[1] = clientWidth * 0.5;
                gStatusBarWidths[2] = -1;

                // 更新状态栏列宽
                SendMessage(gStatusBar, SB_SETPARTS, STATUS_BAR_PARTS, (LPARAM)gStatusBarWidths);
                RefreshWindow(hwnd);
                break;
            }

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                // DrawHexText(hwnd);
                // DrawGrid(hwnd);
                // std::string data = "A";
                // std::string hexStr = AppUtil::StringToHexString(data);
                // gHexString = "0123456789ABCDEF";
                DrawUtil::DrawDataGrid(hwnd, gStatusBar, gFileNameHexStr, gHexString);
                break;
            }

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
} // namespace SlaveWindow end

