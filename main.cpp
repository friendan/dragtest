#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cctype>

// 全局常量
const wchar_t* g_szClassName = L"HexViewWindowClass";
const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
const int PAGE_SIZE = 4096; // 每页显示n字节

// 全局变量
std::vector<unsigned char> g_fileData;    // 存储文件二进制内容
std::wstring g_currentFileName;           // 当前拖拽的文件名
int g_currentPage = 0;                    // 当前页码（从0开始）
HFONT g_hConsoleFont = NULL;              // 控制台等宽字体

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::wstring BinaryToHex(const std::vector<unsigned char>& data, int page) {
    std::wostringstream oss;
    if (data.empty()) {
        return oss.str();
    }

    // 计算当前页的起始/结束位置
    int startIdx = page * PAGE_SIZE;
    if (startIdx >= (int)data.size()) {
        return oss.str();
    }
    int endIdx = (page + 1) * PAGE_SIZE;
    if (endIdx >= (int)data.size()) {
        endIdx = data.size() - 1;
    }
    
    // 关键：先设置流为16进制输出模式 十六进制字母（A-F）显示为大写（如 1A 而非 1a）
    oss << std::hex << std::uppercase;

    for (int i = startIdx; i <= endIdx; i += 1) {
        // 此时输出 (int)data[i]，就会以16进制显示
        // std::setw(2) 确保输出占 2 个字符宽度（不足补 0，如 05 而非 5）
        // std::setfill(L'0')   宽度不足时用 0 填充（而非空格）
        oss << std::setw(2) << std::setfill(L'0') << (unsigned int)data[i];
    }

    return oss.str();
}

// 通用绘制函数（16进制内容自适应显示）
void DrawHexText(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    // 1. 获取当前窗口客户区大小
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    // 2. 黑底白字样式
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    // 3. 控制台等宽字体（必须等宽，否则16进制对齐混乱）
    HFONT hOldFont = NULL;
    if (g_hConsoleFont) {
        hOldFont = (HFONT)SelectObject(hdc, g_hConsoleFont);
    }

    // 4. 生成16进制显示文本
    std::wstring hexText = BinaryToHex(g_fileData, g_currentPage);

    // 5. 自适应绘制（支持自动换行，确保翻页后内容适配窗口）
    RECT rcText = rcClient;
    rcText.left += 10;
    rcText.top += 10;
    rcText.right -= 10;
    rcText.bottom -= 10;

    // DT_EDITCONTROL+DT_WORDBREAK：任意字符换行；DT_LEFT：左对齐；DT_NOCLIP：不裁剪
    UINT drawFlags = DT_WORDBREAK | DT_LEFT | DT_NOCLIP | DT_EDITCONTROL;
    DrawText(hdc, hexText.c_str(), -1, &rcText, drawFlags);

    // 6. 释放资源
    if (hOldFont) SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
}

// 读取文件二进制内容
bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData) {
    outData.clear();
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    // 获取文件大小并分配缓冲区
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(size);
    if (!file.read((char*)outData.data(), size)) {
        outData.clear();
        return false;
    }
    return true;
}

// 注册窗口类
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!RegisterWindowClass(hInstance)) {
        MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        L"Console",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL, // 加垂直滚动条，方便查看多内容
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, L"窗口创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    DragAcceptFiles(hwnd, TRUE); // 启用拖拽

    // 创建等宽字体（Consolas，确保16进制对齐）
    g_hConsoleFont = CreateFont(
        14,                  // 小字号，显示更多内容
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
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

    // 释放资源
    if (g_hConsoleFont) DeleteObject(g_hConsoleFont);
    return (int)msg.wParam;
}

// 窗口过程函数（核心：拖拽、翻页、重绘）
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // 1. 处理文件拖拽
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            wchar_t szFilePath[MAX_PATH] = {0};
            
            // 只读取第一个拖拽的文件（可扩展为多文件）
            if (DragQueryFile(hDrop, 0, szFilePath, MAX_PATH)) {
                g_currentFileName = szFilePath;
                g_currentPage = 0; // 重置页码为第一页
                
                // 读取文件二进制内容
                if (!ReadFileToBinary(g_currentFileName, g_fileData)) {
                    MessageBox(hwnd, L"文件读取失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
                    g_fileData.clear();
                }
            }
            DragFinish(hDrop);

            // 重绘显示16进制内容
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }

        // 2. 处理按键（F2翻页）
        case WM_KEYDOWN: {
            if (wParam == VK_F2) { // F2键
                g_currentPage++; // 页码+1
                // 超出总页数时重置为0（循环翻页，也可改为停在最后一页）
                int totalPages = (g_fileData.size() + PAGE_SIZE - 1) / PAGE_SIZE;
                if (g_currentPage >= totalPages) {
                    g_currentPage = 0;
                }
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
            }
            break;
        }

        // 3. 窗口大小改变时重绘
        case WM_SIZE: {
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        }

        // 4. 系统触发重绘
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            DrawHexText(hwnd);
            break;
        }

        // 5. 窗口销毁
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

