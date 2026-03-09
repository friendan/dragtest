#include <windows.h>
#include <string>
#include <vector>
#include <sstream>

// 全局常量
const wchar_t* g_szClassName = L"DragDropWindowClass";
const int WM_DISPLAY_DRAG_DATA = WM_USER + 1; // 自定义消息，用于更新窗口显示

// 全局变量
std::vector<std::wstring> g_dragPaths;       // 存储拖拽的文件/目录路径
HFONT g_hConsoleFont = NULL;                 // 控制台风格字体

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 辅助函数：拼接路径为显示文本
std::wstring JoinPaths(const std::vector<std::wstring>& paths) {
    std::wostringstream oss;
    for (size_t i = 0; i < paths.size(); ++i) {
        // oss << L"[" << (i + 1) << L"] " << paths[i] << L"\r\n";
        oss << paths[i];
        break;
    }
    return oss.str();
}

// 通用绘制函数（核心：自适应显示逻辑）
void DrawDragText(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    // 1. 获取当前窗口客户区大小（自适应核心）
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    // 2. 设置黑底白字样式
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH)); // 黑色背景
    SetTextColor(hdc, RGB(255, 255, 255));                         // 白色文本
    SetBkMode(hdc, TRANSPARENT);                                   // 透明文本背景

    // 3. 使用控制台等宽字体
    HFONT hOldFont = NULL;
    if (g_hConsoleFont) {
        hOldFont = (HFONT)SelectObject(hdc, g_hConsoleFont);
    }

    // 4. 拼接显示文本
    std::wstring displayText = JoinPaths(g_dragPaths);
    if (g_dragPaths.empty()) {
        // displayText = L"ready..."; // 无拖拽时的提示
    }

    // 5. 自适应绘制文本（关键：DrawText支持自动换行）
    RECT rcText = rcClient;
    rcText.left += 10;
    rcText.top += 10;
    rcText.right -= 10;
    rcText.bottom -= 10;


    // 6. 修复：DrawText参数（关键！DT_EDITCONTROL + DT_WORDBREAK 实现任意字符换行）
    // DT_EDITCONTROL：模拟编辑框换行逻辑，支持任意字符处换行（解决路径无空格不换行问题）
    // DT_WORDBREAK：基础换行
    // DT_LEFT：左对齐
    // DT_NOCLIP：不裁剪文本（确保尾部字符能显示）
    UINT drawFlags = DT_WORDBREAK | DT_LEFT | DT_NOCLIP | DT_EDITCONTROL;
    DrawText(hdc, displayText.c_str(), -1, &rcText, drawFlags);


    // 6. 释放资源
    if (hOldFont) SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
}

// 注册窗口类
BOOL RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = g_szClassName;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // 黑色背景
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    return RegisterClassEx(&wc) != 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!RegisterWindowClass(hInstance)) {
        MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        L"console",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, L"窗口创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    DragAcceptFiles(hwnd, TRUE); // 启用拖拽

    // 创建控制台等宽字体
    g_hConsoleFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_MODERN, L"Consolas"
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 释放字体资源
    if (g_hConsoleFont) DeleteObject(g_hConsoleFont);
    return (int)msg.wParam;
}

// 窗口过程函数（处理拖拽、大小改变、刷新）
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // 1. 处理拖拽放下事件
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            g_dragPaths.clear();

            UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            wchar_t szFilePath[MAX_PATH] = {0};
            for (UINT i = 0; i < fileCount; ++i) {
                if (DragQueryFile(hDrop, i, szFilePath, MAX_PATH)) {
                    g_dragPaths.push_back(szFilePath);
                }
            }
            DragFinish(hDrop);

            // 拖拽后立即重绘
            InvalidateRect(hwnd, NULL, TRUE); // 标记窗口为需要重绘
            UpdateWindow(hwnd);               // 立即重绘
            break;
        }

        // 2. 处理窗口大小改变（核心：自适应触发）
        case WM_SIZE: {
            // 窗口大小改变时重绘文本
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }

        // 3. 处理窗口重绘请求（系统触发：比如窗口被遮挡后恢复）
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps); // 此处仅标记绘制完成，实际绘制在DrawDragText中
            DrawDragText(hwnd);  // 执行自适应绘制
            break;
        }

        // 4. 自定义消息（兼容旧逻辑）
        case WM_DISPLAY_DRAG_DATA: {
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }

        // 5. 窗口销毁
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        // 默认消息处理
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
