#pragma once
#include "MainWindow.h"
#include "AppConstants.h"

namespace MainWindow
{
    const wchar_t* g_szClassName = L"HexViewWindowClass";
    const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
    HBRUSH gBrushList[3];
    HWND gMainWindow = NULL;

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL RegisterWindowClass(HINSTANCE hInstance);
    
    
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

        DragAcceptFiles(gMainWindow, TRUE); // 启用拖拽
        gBrushList[0] = CreateSolidBrush(AppConst::GRID_COLOR_A);
        gBrushList[1] = CreateSolidBrush(AppConst::GRID_COLOR_B);
        gBrushList[2] = CreateSolidBrush(AppConst::GRID_COLOR_C);

        ShowWindow(gMainWindow, nCmdShow);
        UpdateWindow(gMainWindow);

        // 消息循环
        MSG msg = {0};
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        DeleteObject(gBrushList[0]); // 释放画刷资源（必须释放，避免内存泄漏）
        DeleteObject(gBrushList[1]); // 释放画刷资源（必须释放，避免内存泄漏）
        DeleteObject(gBrushList[2]); // 释放画刷资源（必须释放，避免内存泄漏）
    }

    void RefreshWindow(HWND hwnd){
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
            // 1. 处理文件拖拽
            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                wchar_t szFilePath[MAX_PATH] = {0};
                
                // 只读取第一个拖拽的文件（可扩展为多文件）
                if (DragQueryFile(hDrop, 0, szFilePath, MAX_PATH)) {
                    // g_currentFileName = szFilePath;
                    // g_currentPage = 0; // 重置页码为第一页
                    
                    // // 读取文件二进制内容
                    // if (!ReadFileToBinary(g_currentFileName, g_fileData)) {
                    //     MessageBox(hwnd, L"文件读取失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
                    //     g_fileData.clear();
                    // }
                }
                DragFinish(hDrop);
                RefreshWindow(hwnd);
                break;
            }
            case WM_KEYDOWN: {
                if (wParam == VK_F2) { // F2
                    RefreshWindow(hwnd);
                }
                break;
            }
            case WM_SIZE: {
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





} // namespace MainWindow end

