#pragma once
#include "MainWindow.h"
#include "AppConstants.h"
#include "ImageUtil.h"
#include "DrawUtil.h"
#include "AppUtil.h"
#include "ResourceIDs.h"
#include <commctrl.h>
#include <Shlwapi.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace MainWindow
{
    const wchar_t* g_szClassName = L"HexViewWindowClass";
    const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
    const int WM_UPDATE_IMAGE_LIST = WM_USER + 2; // 自定义UI更新消息
    const int WM_ADD_LOG_LINE = WM_USER + 3;
    HWND gMainWindow = NULL;
    HWND gStatusBar = NULL;  // 状态栏句柄
    const int STATUS_BAR_PARTS = 3; // 状态栏列数
    int gStatusBarWidths[STATUS_BAR_PARTS]; // 存储3列宽度

    // C++11 线程相关（替代Windows临界区/线程句柄）
    std::mutex gImageFilesMutex;          // 保护图片列表的互斥锁
    std::atomic<bool> gIsProcessing{false}; // 原子变量：是否正在处理（线程安全）
    std::unique_ptr<std::thread> gWorkerThread; // 智能指针管理线程，自动释放

    // 图片文件信息结构体
    struct ImageFileInfo {
        std::wstring filePath;
        FILETIME createTime;
    };
    std::vector<ImageFileInfo> gImageFiles;

    // 日志相关变量
    const int LOG_MAX_LINES = 10000; // 最大缓存日志行数（防止内存溢出）
    std::vector<std::wstring> gLogLines; // 日志行缓存
    std::mutex gLogMutex;               // 日志操作互斥锁
    HWND gLogScrollBar = NULL;          // 日志区域滚动条
    int gLogLineHeight = 28;            // 每行日志高度（像素）
    int gLogTopOffset = 0;              // 日志滚动偏移量
    RECT gLogAreaRect;                  // 日志显示区域（排除状态栏）
    HFONT gLogFont = NULL;

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL RegisterWindowClass(HINSTANCE hInstance);
    void CreateStatusBar(HWND hwnd); // 创建状态栏
    void RefreshWindow(HWND hwnd);
    bool IsImageFile(const std::wstring& filePath);
    bool CompareImageFileByCreateTime(const ImageFileInfo& a, const ImageFileInfo& b);
    void ProcessFolder(const std::wstring& folderPath, HWND hwnd);
    void ExtractImageData(const std::wstring& folderPath);
    std::string getHexStrFromPixelList(const std::vector<std::vector<ImageUtil::PixelInfo>>& pixelList);
    std::string ColorListToHexString(const std::vector<size_t>& colorList);

     // 初始化日志区域（窗口创建后调用）
    void InitLogArea(HWND hwnd) {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetRect(&gLogAreaRect, 
            rcClient.left + 10,          // 左边距10
            rcClient.top + 10,           // 上边距10
            rcClient.right - 27,         // 右边距=滚动条宽度17+10，让滚动条贴窗口右边缘
            rcClient.bottom - 34);       // 下边距=状态栏高度24+10
        // 创建垂直滚动条（贴窗口右边缘）
        gLogScrollBar = CreateWindowEx(
            0,
            L"SCROLLBAR",
            NULL,
            WS_CHILD | WS_VISIBLE | SBS_VERT,
            rcClient.right - 17,         // 滚动条x坐标：窗口右边缘-17（贴边）
            gLogAreaRect.top,            // 滚动条y坐标
            17,                          // 滚动条宽度
            gLogAreaRect.bottom - gLogAreaRect.top, // 滚动条高度
            hwnd,
            (HMENU)1002,                 // 滚动条ID
            GetModuleHandle(NULL),
            NULL
        );
        SendMessage(gLogScrollBar, SBM_SETRANGE, 0, MAKELPARAM(0, 0));

        // ========== 创建全局日志字体（仅一次） ==========
        HDC hdc = GetDC(hwnd);
        int pointSize = 12;
        int logpixelsy = GetDeviceCaps(hdc, LOGPIXELSY);
        int fontHeight = -MulDiv(pointSize, logpixelsy, 72);
        gLogFont = CreateFont(
            fontHeight,                // 14pt高度
            0,                         // 宽度自动
            0, 0,                      // 角度
            FW_NORMAL,                 // 常规粗细
            FALSE, FALSE, FALSE,       // 无特殊样式
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,       // 抗锯齿
            FF_DONTCARE,
            L"微软雅黑"                // 字体名
        );
        ReleaseDC(hwnd, hdc);

        // 容错：创建失败则用系统默认字体
        if (gLogFont == NULL) {
            gLogFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }


    // 添加日志行（线程安全，支持多线程调用）
    void AddLogLine(const std::wstring& logLine) {
        std::lock_guard<std::mutex> lock(gLogMutex);
        SYSTEMTIME st = {0};
        GetLocalTime(&st);

        // 格式化时间为 "YYYY-MM-DD HH:MM:SS " 格式（宽字符串）
        wchar_t timePrefix[32] = {0};
        wsprintf(timePrefix, 
            L"%04d-%02d-%02d %02d:%02d:%02d ", 
            st.wYear, st.wMonth, st.wDay, 
            st.wHour, st.wMinute, st.wSecond);

        std::wstring logWithTime = timePrefix + logLine;
    
        // 添加日志行、裁剪最大行数
        gLogLines.push_back(logWithTime);
        if (gLogLines.size() > LOG_MAX_LINES) {
            gLogLines.erase(gLogLines.begin());
            gLogTopOffset = max(0, gLogTopOffset - 1);
        }

        // 计算滚动条最大位置
        int visibleLines = (gLogAreaRect.bottom - gLogAreaRect.top) / gLogLineHeight;
        int maxScrollPos = max(0, (int)gLogLines.size() - visibleLines);
        
        // 日志行数 <= 可视行数时，隐藏滚动条；否则显示
        if (gLogLines.size() <= visibleLines) {
            ShowWindow(gLogScrollBar, SW_HIDE);
        } else {
            ShowWindow(gLogScrollBar, SW_SHOW);
            SendMessage(gLogScrollBar, SBM_SETRANGE, 0, MAKELPARAM(0, maxScrollPos));
            SendMessage(gLogScrollBar, SBM_SETPOS, gLogTopOffset, 0);
        }

        PostMessage(gMainWindow, WM_ADD_LOG_LINE, 0, 0);
    }

    // 重载：支持窄字符串
    void AddLogLine(const std::string& logLine) {
        AddLogLine(AppUtil::Utf8ToUtf16(logLine)); // 需确保AppUtil有Utf8ToUtf16转换函数
    }

    // 绘制日志（在WM_PAINT中调用）
    void DrawLog(HDC hdc) {
       // 线程安全锁
        std::lock_guard<std::mutex> lock(gLogMutex);
        if (gLogLines.empty()) return;

        // ======= 复用全局字体（仅选入，不重复创建） ==========
        if (gLogFont == NULL) { // 极端容错：字体未创建则用默认
            gLogFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        HFONT hOldFont = (HFONT)SelectObject(hdc, gLogFont);

        // ========== 设置文本样式 ==========
        SetTextColor(hdc, RGB(0, 255, 0));  // 绿色文字
        SetBkColor(hdc, RGB(0, 0, 0));      // 黑色背景
        SetBkMode(hdc, OPAQUE);             // 不透明

        // ========== 计算可视行数 ==========
        int visibleLines = (gLogAreaRect.bottom - gLogAreaRect.top) / gLogLineHeight;
        int startLine = gLogTopOffset;
        int endLine = min(startLine + visibleLines, (int)gLogLines.size());

        // ========== 逐行绘制 ==========
        int yPos = gLogAreaRect.top;
        for (int i = startLine; i < endLine; ++i) {
            TextOut(
                hdc,
                gLogAreaRect.left + 5,
                yPos,
                gLogLines[i].c_str(),
                (int)gLogLines[i].length()
            );
            yPos += gLogLineHeight;
        }

        // ========== 恢复旧字体（仅恢复，不删除全局字体） ==========
        SelectObject(hdc, hOldFont);
    }

    // 处理滚动条消息
    void HandleScrollMessage(HWND hwnd, WPARAM wParam, LPARAM lParam) {
        int scrollCode = LOWORD(wParam);
        int newPos = gLogTopOffset;
        int maxPos = max(0, (int)gLogLines.size() - (gLogAreaRect.bottom - gLogAreaRect.top) / gLogLineHeight);

        // 根据滚动操作计算新位置
        switch (scrollCode) {
            case SB_LINEUP:    newPos = max(0, newPos - 1); break;
            case SB_LINEDOWN:  newPos = min(maxPos, newPos + 1); break;
            case SB_PAGEUP:    newPos = max(0, newPos - 10); break;
            case SB_PAGEDOWN:  newPos = min(maxPos, newPos + 10); break;
            case SB_THUMBPOSITION: 
            case SB_THUMBTRACK: newPos = HIWORD(wParam); break;
            case SB_TOP:       newPos = 0; break;
            case SB_BOTTOM:    newPos = maxPos; break;
        }

        // 更新滚动位置并刷新
        if (newPos != gLogTopOffset) {
            gLogTopOffset = newPos;
            SendMessage(gLogScrollBar, SBM_SETPOS, newPos, 0);
            RefreshWindow(hwnd);
        }
    }
    
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

    void processImageTest(){
        std::string testImgPath = ".\\test\\0.png";
        std::vector<std::vector<ImageUtil::PixelInfo>> pixelList = ImageUtil::TraverseImagePixels(testImgPath);
        getHexStrFromPixelList(pixelList);
    }

    void PrintTest(){
        // ========== t0=0 的所有有效组合 ==========
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][0][0] = ", AppConst::TRIAD_TO_DEC_3D[0][0][0]); // 0
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][0][1] = ", AppConst::TRIAD_TO_DEC_3D[0][0][1]); // 1
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][0][2] = ", AppConst::TRIAD_TO_DEC_3D[0][0][2]); // 2
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][1][0] = ", AppConst::TRIAD_TO_DEC_3D[0][1][0]); // 3
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][1][1] = ", AppConst::TRIAD_TO_DEC_3D[0][1][1]); // 4
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][1][2] = ", AppConst::TRIAD_TO_DEC_3D[0][1][2]); // 5
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][2][0] = ", AppConst::TRIAD_TO_DEC_3D[0][2][0]); // 6
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][2][1] = ", AppConst::TRIAD_TO_DEC_3D[0][2][1]); // 7
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[0][2][2] = ", AppConst::TRIAD_TO_DEC_3D[0][2][2]); // 8

        // ========== t0=1 的所有有效组合 ==========
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][0][0] = ", AppConst::TRIAD_TO_DEC_3D[1][0][0]); // 9
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][0][1] = ", AppConst::TRIAD_TO_DEC_3D[1][0][1]); // 10
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][0][2] = ", AppConst::TRIAD_TO_DEC_3D[1][0][2]); // 11
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][1][0] = ", AppConst::TRIAD_TO_DEC_3D[1][1][0]); // 12
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][1][1] = ", AppConst::TRIAD_TO_DEC_3D[1][1][1]); // 13
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][1][2] = ", AppConst::TRIAD_TO_DEC_3D[1][1][2]); // 14
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][2][0] = ", AppConst::TRIAD_TO_DEC_3D[1][2][0]); // 15

        // ========== 可选：打印无效组合（验证-1） ==========
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[1][2][1] = ", AppConst::TRIAD_TO_DEC_3D[1][2][1]); // -1
        AppUtil::SaveLog("TRIAD_TO_DEC_3D[2][0][0] = ", AppConst::TRIAD_TO_DEC_3D[2][0][0]); // -1
    }

    void StartMainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
        if (!RegisterWindowClass(hInstance)) {
            MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        gMainWindow = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            g_szClassName,
            L"GridTriad",
            // WS_OVERLAPPEDWINDOW | WS_VSCROLL, // 加垂直滚动条，方便查看多内容
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
            NULL, NULL, hInstance, NULL
        );

        if (!gMainWindow) {
            MessageBox(NULL, L"窗口创建失败！", L"错误", MB_ICONEXCLAMATION | MB_OK);
            return;
        }

        ImageUtil::InitGdiplus();

        // 创建状态栏（窗口创建后立即创建）
        CreateStatusBar(gMainWindow);
        DragAcceptFiles(gMainWindow, TRUE); // 启用拖拽
        DrawUtil::InitDraw();
        // InitLogArea(gMainWindow);

        ShowWindow(gMainWindow, nCmdShow);
        UpdateWindow(gMainWindow);
        SetWindowPos(gMainWindow, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
        
        // test
        DeleteFileW(L"app.log");
        // PrintTest();
        // std::thread imgProcess(processImageTest);
        // imgProcess.detach();

        AddLogLine(L"ready...");

        // 消息循环
        MSG msg = {0};
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 窗口退出：等待线程结束（最多5秒）
        if (gWorkerThread && gWorkerThread->joinable()) {
            gWorkerThread->join(); // 等待线程完成
        }
        
        DrawUtil::UnInitDraw();
        ImageUtil::ShutdownGdiplus();
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
            // 窗口创建时（可选，也可在StartMainWindow中创建状态栏）
            case WM_CREATE:{
                static HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
                if (hIcon) {
                    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);      // 设置窗口大图标
                    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);    // 设置窗口小图标（标题栏）
                }
                InitLogArea(hwnd);
                break;
            }

            case WM_ADD_LOG_LINE: {
                RefreshWindow(hwnd);
                break;
            }

            case WM_VSCROLL: {
                HandleScrollMessage(hwnd, wParam, lParam);
                break;
            }

            // 处理文件拖拽（仅创建线程，无耗时操作）
            case WM_DROPFILES: {
                HDROP hDrop = (HDROP)wParam;
                wchar_t szPath[MAX_PATH] = {0};
                if (DragQueryFile(hDrop, 0, szPath, MAX_PATH)) {
                    DWORD attr = GetFileAttributes(szPath);
                    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                        // 原子判断：防止重复处理（线程安全）
                        if (gIsProcessing.load()) {
                            MessageBox(hwnd, L"正在处理文件夹，请稍候！", L"提示", MB_ICONINFORMATION | MB_OK);
                            DragFinish(hDrop);
                            return 0;
                        }

                        // 设置处理中状态（原子操作）
                        gIsProcessing = true;
                        // 更新状态栏提示
                        SendMessage(gStatusBar, SB_SETTEXT, 0, (LPARAM)L"working...");
                        SendMessage(gStatusBar, SB_SETTEXT, 1, (LPARAM)L"waiting");
                        RefreshWindow(hwnd);

                        // C++11 创建线程：用智能指针管理，自动释放
                        gWorkerThread = std::make_unique<std::thread>(
                            &MainWindow::ProcessFolder, // 线程函数
                            std::wstring(szPath),       // 文件夹路径（值传递，避免悬空指针）
                            hwnd                        // 窗口句柄
                        );

                        // 分离线程？不！用joinable()在窗口销毁时join，避免内存泄漏
                        // gWorkerThread->detach(); // 不推荐，建议用智能指针+join
                    } else {
                        // MessageBox(hwnd, L"请拖拽文件夹（而非单个文件）！", L"提示", MB_ICONINFORMATION | MB_OK);
                    }
                }
                DragFinish(hDrop);
                break;
            }

            // 处理线程完成后的UI更新（主线程执行）
            case WM_UPDATE_IMAGE_LIST: {
                // 接收堆分配的指针
                std::unique_ptr<std::wstring> pFolderPath(reinterpret_cast<std::wstring*>(lParam));
                if (!pFolderPath) break; 
                
                // 更新状态栏
                if (gStatusBar) {
                    // 图片数量（加锁读取）
                    std::lock_guard<std::mutex> lock(gImageFilesMutex);
                    int imageCount = (int)gImageFiles.size();
                    wchar_t countText[50] = {0};
                    wsprintf(countText, L"images:%d", imageCount);
                    SendMessage(gStatusBar, SB_SETTEXT, 0, (LPARAM)countText);

                    SendMessage(gStatusBar, SB_SETTEXT, 1, (LPARAM)L"finish");
                    SendMessage(gStatusBar, SB_SETTEXT, 2, (LPARAM)pFolderPath->c_str());
                }

                // 刷新窗口
                RefreshWindow(hwnd);
                break;
            }

            case WM_KEYDOWN: {
                if (wParam == VK_F2) { // F2
                    RefreshWindow(hwnd);
                    // UpdateStatusBarText(2, L"已刷新 | F2 刷新 | 拖拽文件到窗口"); // 临时更新第3列
                }
                else if(wParam == VK_RETURN){
                    std::string tmpLogStr = AppUtil::GetRandHexString(AppUtil::GetRandNumber(16, 1024));
                    AddLogLine(tmpLogStr);
                }
                else if(wParam == VK_ESCAPE){
                    std::lock_guard<std::mutex> lock(gLogMutex);
                    gLogLines.clear();
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

                 // 调整日志区域和滚动条（修复后）
                SetRect(&gLogAreaRect, 
                    10, 10, 
                    clientWidth - 27, // 日志区域右边界=窗口宽度-滚动条宽度17-10边距
                    clientHeight - 34);
                
                if (gLogScrollBar) {
                    // 滚动条贴窗口右边缘
                    MoveWindow(gLogScrollBar, 
                        clientWidth - 17,    // x坐标：窗口右边缘-17
                        gLogAreaRect.top,    // y坐标
                        17,                  // 宽度
                        gLogAreaRect.bottom - gLogAreaRect.top, // 高度
                        TRUE);
                }

                // 更新滚动条范围
                int maxScrollPos = max(0, (int)gLogLines.size() - (gLogAreaRect.bottom - gLogAreaRect.top) / gLogLineHeight);
                SendMessage(gLogScrollBar, SBM_SETRANGE, 0, MAKELPARAM(0, maxScrollPos));
                RefreshWindow(hwnd);
                break;
            }

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                DrawLog(hdc);
                EndPaint(hwnd, &ps);
                // DrawHexText(hwnd);
                // DrawGrid(hwnd);
                break;
            }

            case WM_DESTROY:
                // 窗口销毁：等待线程结束，避免资源泄漏
                if (gWorkerThread && gWorkerThread->joinable()) {
                    gWorkerThread->join();
                }
                PostQuitMessage(0);
                if (gLogFont) DeleteObject(gLogFont); // 释放字体
                break;

            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

     // 判断是否为图片文件
    bool IsImageFile(const std::wstring& filePath) {
        std::wstring ext = PathFindExtension(filePath.c_str());
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        const std::vector<std::wstring> imageExts = {
            L".jpg", L".jpeg", L".png", L".bmp", L".gif", 
            L".tiff", L".tif", L".webp", L".ico"
        };
        return std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end();
    }

    // 排序比较函数
    bool CompareImageFileByCreateTime(const ImageFileInfo& a, const ImageFileInfo& b) {
        if (a.createTime.dwHighDateTime != b.createTime.dwHighDateTime) {
            return a.createTime.dwHighDateTime < b.createTime.dwHighDateTime;
        }
        return a.createTime.dwLowDateTime < b.createTime.dwLowDateTime;
    }

    // C++11 工作线程函数：处理文件夹遍历（无UI操作）
    void ProcessFolder(const std::wstring& folderPath, HWND hwnd) {
        AppUtil::SaveLog("ProcessFolder start");
        std::vector<ImageFileInfo> tempImageFiles;
        // 遍历文件夹
        std::wstring searchPath = folderPath + L"\\*.*";
        WIN32_FIND_DATA findData = {0};
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
                    continue;
                }
                std::wstring fullPath = folderPath + L"\\" + findData.cFileName;
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // 可选：递归遍历子文件夹
                    // ProcessFolder(fullPath, hwnd);
                    continue;
                }
                if (IsImageFile(fullPath)) {
                    ImageFileInfo fileInfo;
                    fileInfo.filePath = fullPath;
                    fileInfo.createTime = findData.ftCreationTime;
                    tempImageFiles.push_back(fileInfo);
                    // AppUtil::SaveLog(fullPath);
                }

            } while (FindNextFile(hFind, &findData));
            FindClose(hFind);
        }

        // 排序
        std::sort(tempImageFiles.begin(), tempImageFiles.end(), CompareImageFileByCreateTime);

        // 线程安全：更新全局列表（加互斥锁）
        {
            std::lock_guard<std::mutex> lock(gImageFilesMutex); // RAII自动解锁
            gImageFiles = tempImageFiles;
        }
       
        // 【修复】在堆上复制字符串，避免局部变量销毁导致指针失效
        // 使用 new wchar_t[] 或者 std::wstring*
        std::wstring* pPath = new std::wstring(folderPath);

        // 【修复】使用 PostMessage 代替 SendMessage
        // PostMessage 立即返回，不会阻塞子线程，防止死锁
        if (!PostMessage(hwnd, WM_UPDATE_IMAGE_LIST, 0, (LPARAM)pPath)) {
            // 如果投递失败（例如窗口已销毁），清理内存
            delete pPath;
        }

        for(auto& fileInfo : MainWindow::gImageFiles){
            AppUtil::SaveLog(fileInfo.filePath);
        }

        AppUtil::SaveLog("gImageFiles.size() ", gImageFiles.size());
        ExtractImageData(folderPath);

        // 重置处理状态（原子操作，线程安全）
        gIsProcessing = false;
        AppUtil::SaveLog("ProcessFolder finish");
    }

    void ExtractImageData(const std::wstring& folderPath){
        if(gImageFiles.size() < 1) return;
        AppUtil::SaveLogW(L"folderPath ", folderPath);
        std::string strDir = AppUtil::Utf16ToUtf8(folderPath);
        std::filesystem::path dirPath(strDir);
        std::filesystem::path filePath;
        std::ostringstream hexStrStream;
        bool parseFileName = true;

        for(auto& fileInfo : MainWindow::gImageFiles){
            AppUtil::SaveLog(fileInfo.filePath);
            AppUtil::UpdateStatusBarText(gStatusBar, 2, fileInfo.filePath.c_str());
            std::string imagePath = AppUtil::Utf16ToUtf8(fileInfo.filePath);
            std::vector<std::vector<ImageUtil::PixelInfo>> pixelList = ImageUtil::TraverseImagePixels(imagePath);
            AppUtil::SaveLog("pixelList.size() ", pixelList.size());
            if(parseFileName){
                parseFileName = false;
                std::string tmpHexStr = getHexStrFromPixelList(pixelList);
                std::string dstFileName = AppUtil::hexStrToStr(tmpHexStr);
                AppUtil::SaveLog("dstFileName ", dstFileName);
                if(dstFileName.empty()){
                    return;
                }
                filePath = dirPath / dstFileName;
                AppUtil::SaveLog("filePath ", filePath.string());
                std::ofstream file(filePath.string(), std::ios::out|std::ios::trunc);
                file.close();
            }else{
                std::string hexStr = getHexStrFromPixelList(pixelList);
                hexStrStream << hexStr;
            }
        }

        AppUtil::hexStrToFile(hexStrStream.str(), filePath.string());
        AppUtil::UpdateStatusBarText(gStatusBar, 2, std::string("ok finish"));
    }

    std::string getHexStrFromPixelList(const std::vector<std::vector<ImageUtil::PixelInfo>>& pixelList){
        std::string hexStr;
        std::ostringstream hexStrStream;
        size_t rowSize = pixelList.size();
        if(rowSize < 10) return hexStr;

        size_t maxCol = GetMaxColumnCount(pixelList);
        size_t gridSize = AppConst::GRID_SIZE;
        std::vector<size_t> colorList;
        std::vector<size_t> colorListLast;
        std::vector<size_t> abcGridList;
        int blackTotal = 0;
        int aColorTotal = 0;
        int bColorTotal = 0;
        int cColorTotal = 0;
        int otherColorTotal = 0;
        int abcColorTotal = 0;
        size_t findColTotal = 0;

        // AppUtil::SaveLog("AppConst::COLOR_BLACK ",  AppConst::COLOR_BLACK);
        // AppUtil::SaveLog("AppConst::GRID_COLOR_A ", AppConst::GRID_COLOR_A);
        // AppUtil::SaveLog("AppConst::GRID_COLOR_B ", AppConst::GRID_COLOR_B);
        // AppUtil::SaveLog("AppConst::GRID_COLOR_C ", AppConst::GRID_COLOR_C);
        
        for (size_t col = 0; col < maxCol; ++col) {
            blackTotal = 0;
            aColorTotal = 0;
            bColorTotal = 0;
            cColorTotal = 0;
            otherColorTotal = 0;
            abcColorTotal = 0;
            colorList.clear();

            for (size_t row = 0; row < pixelList.size(); ++row) {
                const auto& rowPixels = pixelList[row];
                if (col >= rowPixels.size()) {
                    continue;
                }
                const ImageUtil::PixelInfo& pixel = rowPixels[col];
                // if(col == 10){
                //     AppUtil::SaveLog("col ", col
                //         , " row ", row
                //         , " color ", pixel.color
                //         , " ", blackTotal
                //         , " ", aColorTotal
                //         , " ", bColorTotal
                //         , " ", cColorTotal
                //     );
                // }
                
                switch(pixel.color){
                    case AppConst::COLOR_BLACK: {
                        blackTotal += 1;

                        if(aColorTotal >= gridSize){
                            colorList.push_back(0);
                        }
                        if(bColorTotal >= gridSize){
                            colorList.push_back(1);
                        }
                        if(cColorTotal >= gridSize){
                            colorList.push_back(2);
                        }

                        aColorTotal = 0;
                        bColorTotal = 0;
                        cColorTotal = 0;
                        // AppUtil::SaveLog("case AppConst::COLOR_BLACK ", blackTotal);
                        break;
                    }
                    case AppConst::GRID_COLOR_A: {
                        aColorTotal += 1;
                        if(blackTotal < gridSize){
                            aColorTotal = 0;
                        }
                        // AppUtil::SaveLog("case AppConst::GRID_COLOR_A ", blackTotal, " ", aColorTotal);
                        break;
                    }
                    case AppConst::GRID_COLOR_B:{
                        bColorTotal += 1;
                        if(blackTotal < gridSize){
                            bColorTotal = 0;
                        }
                        // AppUtil::SaveLog("case AppConst::GRID_COLOR_B ", blackTotal, " ", bColorTotal);
                        break;
                    }
                    case AppConst::GRID_COLOR_C: {
                        cColorTotal += 1;
                        if(blackTotal < gridSize){
                            cColorTotal = 0;
                        }
                        // AppUtil::SaveLog("case AppConst::GRID_COLOR_C ", blackTotal, " ", cColorTotal);
                        break;
                    }
                    default: {
                        otherColorTotal = otherColorTotal + 1;
                        // AppUtil::SaveLog("case default ", blackTotal, " gridSize ", gridSize);
                        if(blackTotal < gridSize){
                            // AppUtil::SaveLog("col ", col, " row ", row, " blackTotal ", blackTotal, " gridSize ", gridSize);
                            blackTotal = 0;
                            aColorTotal = 0;
                            bColorTotal = 0;
                            cColorTotal = 0;
                        }
                    }
                }
            }

            abcColorTotal = aColorTotal + bColorTotal + cColorTotal;
            if(otherColorTotal > (blackTotal + abcColorTotal)){
                continue; // 不是很准确？
            }

            if(colorList.size() >= 3){
                findColTotal += 1;
                colorListLast = colorList;
            }else{
                if(findColTotal >= gridSize){
                    abcGridList.insert(abcGridList.end(), colorListLast.begin(), colorListLast.end());
                    // AppUtil::SaveLog("findColGrid col ", col, " gridNum ", colorListLast.size());
                }
                findColTotal = 0;
            }

            /*
            AppUtil::SaveLog("col ", col
                , " colorTotal ", blackTotal
                , " ", aColorTotal
                , " ", bColorTotal
                , " ", cColorTotal
                , " ", otherColorTotal
                , " findColTotal ", findColTotal
            );
            */

            // if(col >= 80) break; // for test
        }

        AppUtil::SaveLog("ColorListToHexString abcGridList.size() ", abcGridList.size());
        hexStrStream << ColorListToHexString(abcGridList);

        hexStr = hexStrStream.str();
        // AppUtil::SaveLog("hexStr ", hexStr);
        // AppUtil::SaveLog("hexStrToStr ", AppUtil::hexStrToStr(hexStr));
        return hexStr;
    }

    std::string ColorListToHexString(const std::vector<size_t>& colorList){
        std::string hexStr;
        std::ostringstream hexStrStream;
        if(colorList.size() < 3 || colorList.size() % 3 != 0){
            return hexStr;
        }
        size_t dec = 0;
        size_t a = 0;
        size_t b = 0;
        size_t c = 0;
        for(size_t idx = 0; idx < colorList.size(); ){
            a = colorList[idx++];
            b = colorList[idx++];
            c = colorList[idx++];
            dec = AppConst::TRIAD_TO_DEC_3D[a][b][c];
            hexStrStream << AppConst::DEC_TO_HEX_TABLE[dec];
        }
        hexStr = hexStrStream.str();
        return hexStr;
    }


} // namespace MainWindow end