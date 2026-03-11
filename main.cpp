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
const int PAGE_SIZE = 512; // 每页显示n字节
const int GRID_SIZE = 16;


// 全局变量
std::vector<unsigned char> g_fileData;    // 存储文件二进制内容
std::wstring g_currentFileName;           // 当前拖拽的文件名
int g_currentPage = 0;                    // 当前页码（从0开始）
HFONT g_hConsoleFont = NULL;              // 控制台等宽字体
// HBRUSH g_hBrush_255_255_255 = CreateSolidBrush(RGB(255, 0, 0));
// HBRUSH g_hBrush_255_255_0 = CreateSolidBrush(RGB(255, 0, 0));
// HBRUSH g_hBrush_0_255_255 = CreateSolidBrush(RGB(255, 0, 0));
HBRUSH gBrushList[3];

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 核心：0-255 每个字节对应唯一、易识别的汉字（无复用）
// 选字原则：笔画简单、无形近字（如 零/一/二 而非 林/琳/淋）、OCR 识别无歧义
wchar_t byteToHanzi(unsigned char byte) {
    // 0-255 完整映射表（按顺序排列，每个位置对应一个字节值）
    const wchar_t hanziTable[256] = {
        // 0-9
        L'零', L'一', L'二', L'三', L'四', L'五', L'六', L'七', L'八', L'九',
        // 10-19
        L'十', L'壹', L'贰', L'叁', L'肆', L'伍', L'陆', L'柒', L'捌', L'玖',
        // 20-29
        L'廿', L'甲', L'乙', L'丙', L'丁', L'戊', L'己', L'庚', L'辛', L'壬',
        // 30-39
        L'癸', L'子', L'丑', L'寅', L'卯', L'辰', L'巳', L'午', L'未', L'申',
        // 40-49
        L'酉', L'戌', L'亥', L'金', L'木', L'水', L'火', L'土', L'日', L'月',
        // 50-59
        L'星', L'风', L'云', L'雷', L'雨', L'山', L'石', L'田', L'禾', L'米',
        // 60-69
        L'竹', L'瓜', L'果', L'花', L'草', L'虫', L'鱼', L'鸟', L'兽', L'人',
        // 70-79
        L'口', L'手', L'足', L'目', L'耳', L'鼻', L'舌', L'牙', L'眉', L'发',
        // 80-89
        L'心', L'肝', L'脾', L'肺', L'肾', L'头', L'身', L'背', L'腰', L'腿',
        // 90-99
        L'脚', L'天', L'地', L'光', L'电', L'气', L'声', L'香', L'味', L'触',
        // 100-109
        L'刀', L'弓', L'车', L'舟', L'戈', L'斤', L'爪', L'瓦', L'皿', L'网',
        // 110-119
        L'巾', L'衣', L'食', L'住', L'行', L'马', L'牛', L'羊', L'犬', L'豕',
        // 120-129
        L'鹿', L'象', L'虎', L'龙', L'凤', L'龟', L'蛇', L'蛙', L'蝉', L'蜂',
        // 130-139
        L'燕', L'雀', L'鸦', L'鸽', L'鸡', L'鸭', L'鹅', L'猫', L'兔', L'狐',
        // 140-149
        L'狸', L'熊', L'狼', L'豹', L'狮', L'猴', L'猿', L'鼠', L'蚁', L'蛾',
        // 150-159
        L'蝶', L'蚊', L'蝇', L'虾', L'蟹', L'贝', L'蚌', L'螺', L'蚪', L'苗',
        // 160-169
        L'芽', L'根', L'叶', L'枝', L'干', L'皮', L'毛', L'骨', L'血', L'汗',
        // 170-179
        L'泪', L'油', L'酒', L'茶', L'汤', L'盐', L'糖', L'醋', L'酱', L'椒',
        // 180-189
        L'葱', L'姜', L'蒜', L'瓜', L'果', L'桃', L'李', L'杏', L'梨', L'枣',
        // 190-199
        L'橘', L'橙', L'柚', L'梅', L'兰', L'菊', L'荷', L'莲', L'菱', L'藕',
        // 200-209
        L'麦', L'豆', L'麻', L'棉', L'丝', L'布', L'纱', L'线', L'针', L'钉',
        // 210-219
        L'锤', L'锯', L'斧', L'铲', L'锄', L'犁', L'耙', L'镰', L'刀', L'枪',
        // 220-229
        L'剑', L'弓', L'箭', L'盾', L'甲', L'盔', L'袍', L'靴', L'帽', L'袜',
        // 230-239
        L'鞋', L'裤', L'裙', L'衫', L'袄', L'被', L'褥', L'枕', L'席', L'帘',
        // 240-249
        L'窗', L'门', L'墙', L'房', L'屋', L'楼', L'梯', L'阶', L'庭', L'院',
        // 250-255
        L'巷', L'街', L'路', L'桥', L'河', L'海'
    };

    // 直接返回对应字节的汉字（byte 范围 0-255，无越界风险）
    return hanziTable[byte];
}

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

    for (int i = startIdx; i < endIdx; i += 1) {
        // 此时输出 (int)data[i]，就会以16进制显示
        // std::setw(2) 确保输出占 2 个字符宽度（不足补 0，如 05 而非 5）
        // std::setfill(L'0')   宽度不足时用 0 填充（而非空格）
        unsigned byte = (unsigned int)data[i];
        wchar_t hanzi = byteToHanzi(byte); // 映射为唯一汉字
        // oss << std::setw(2) << std::setfill(L'0') << byte << " ";
        oss << hanzi << " ";
    }

    return oss.str();
}


void DrawGrid(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
    int width  = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;
    int startX = 16;
    int startY = 16;
    int xMax = width - GRID_SIZE*2;
    int yMax = height - GRID_SIZE*2;

    int color = 0;
    RECT rect;
    int colCount = 0;
    int xOffset = 0;
    for(int x = startX; x < xMax; x += GRID_SIZE){
        for(int y = startY; y < yMax; y += GRID_SIZE){
            rect.left   = x + xOffset; 
            rect.top    = y;
            rect.right  = x + GRID_SIZE + xOffset;
            rect.bottom = y + GRID_SIZE;
            FillRect(hdc, &rect, gBrushList[color%3]);
            color = color + 1;
        }
        colCount = colCount + 1;
        xOffset = xOffset + GRID_SIZE;
        if(colCount >= 6){
            // break;
        }
    }
   
    // HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
    // HBRUSH hRedBrush = CreateSolidBrush(RGB(0, 255, 0));
    // HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 255, 255));
    // RECT rect;
    // rect.left   = 50;    // 左边界
    // rect.top    = 50;    // 上边界
    // rect.right  = 50 + GRID_SIZE;// 右边界（左+宽度）
    // rect.bottom = 50 + GRID_SIZE;// 下边界（上+高度）
    // FillRect(hdc, &rect, hRedBrush);
    // DeleteObject(hRedBrush); // 释放画刷资源（必须释放，避免内存泄漏）


    ReleaseDC(hwnd, hdc);
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
    int offset = 20;
    RECT rcText = rcClient;
    rcText.left += offset;
    rcText.top += offset;
    rcText.right -= offset;
    rcText.bottom -= offset;

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

    gBrushList[0] = CreateSolidBrush(RGB(255, 0, 0));
    gBrushList[1] = CreateSolidBrush(RGB(0, 255, 0));
    gBrushList[2] = CreateSolidBrush(RGB(255, 255, 255));

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
        30,                  // 小字号，显示更多内容
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        // FIXED_PITCH | FF_MODERN, L"Consolas"
        FIXED_PITCH | FF_MODERN, L"微软雅黑"
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
                    // g_currentPage = 0;
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
            // DrawHexText(hwnd);
            DrawGrid(hwnd);
            break;
        }

        // 5. 窗口销毁
        case WM_DESTROY:
            PostQuitMessage(0);
            DeleteObject(gBrushList[0]); // 释放画刷资源（必须释放，避免内存泄漏）
            DeleteObject(gBrushList[1]); // 释放画刷资源（必须释放，避免内存泄漏）
            DeleteObject(gBrushList[2]); // 释放画刷资源（必须释放，避免内存泄漏）
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

