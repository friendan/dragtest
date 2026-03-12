#pragma once
#include "SlaveWindow.h"
#include "AppConstants.h"
#include <commctrl.h>

namespace SlaveWindow
{
    const wchar_t* g_szClassName = L"HexViewWindowClass";
    const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
    HBRUSH gBrushList[3];
    HWND gMainWindow = NULL;
    HWND gStatusBar = NULL;  // 状态栏句柄
    const int STATUS_BAR_PARTS = 3; // 状态栏列数
    int gStatusBarWidths[STATUS_BAR_PARTS]; // 存储3列宽度

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

    void StartMainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
        if (!RegisterWindowClass(hInstance)) {
            MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        gMainWindow = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            g_szClassName,
            L"Console",
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
        gBrushList[0] = CreateSolidBrush(AppConst::GRID_COLOR_A);
        gBrushList[1] = CreateSolidBrush(AppConst::GRID_COLOR_B);
        gBrushList[2] = CreateSolidBrush(AppConst::GRID_COLOR_C);

        // 初始化状态栏文本
        UpdateStatusBarText(0, L"未选择文件");
        UpdateStatusBarText(1, L"大小：0 字节");
        UpdateStatusBarText(2, L"F2 刷新 | 拖拽文件到窗口");

        ShowWindow(gMainWindow, nCmdShow);
        UpdateWindow(gMainWindow);

        // 消息循环
        MSG msg = {0};
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        DeleteObject(gBrushList[0]); // 释放画刷资源
        DeleteObject(gBrushList[1]);
        DeleteObject(gBrushList[2]);
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

    // 更新状态栏指定列的文本
    void UpdateStatusBarText(int partIndex, const wchar_t* text) {
        if (gStatusBar == NULL || partIndex < 0 || partIndex >= STATUS_BAR_PARTS) {
            return;
        }
        // SB_SETTEXT：设置指定列文本；0：绘制方式（普通）
        SendMessage(gStatusBar, SB_SETTEXT, partIndex, (LPARAM)text);
    }

    void RefreshWindow(HWND hwnd){
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
            // 窗口创建时（可选，也可在StartMainWindow中创建状态栏）
            case WM_CREATE:
                break;

            // 处理文件拖拽
            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                wchar_t szFilePath[MAX_PATH] = {0};
                
                // 只读取第一个拖拽的文件
                if (DragQueryFile(hDrop, 0, szFilePath, MAX_PATH)) {
                    // 示例：更新状态栏文本（实际可读取文件大小后更新）
                    UpdateStatusBarText(0, szFilePath); // 第1列显示文件名
                    
                    // 可选：获取文件大小
                    WIN32_FILE_ATTRIBUTE_DATA fileData;
                    if (GetFileAttributesEx(szFilePath, GetFileExInfoStandard, &fileData)) {
                        ULONGLONG fileSize = (ULONGLONG)fileData.nFileSizeHigh << 32 | fileData.nFileSizeLow;
                        wchar_t szSizeText[50] = {0};
                        wsprintf(szSizeText, L"大小：%llu 字节", fileSize);
                        UpdateStatusBarText(1, szSizeText); // 第2列显示文件大小
                    }
                }
                DragFinish(hDrop);
                RefreshWindow(hwnd);
                break;
            }

            case WM_KEYDOWN: {
                if (wParam == VK_F2) { // F2
                    RefreshWindow(hwnd);
                    // UpdateStatusBarText(2, L"已刷新 | F2 刷新 | 拖拽文件到窗口"); // 临时更新第3列
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

