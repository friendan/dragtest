#pragma once
#include "MainWindow.h"
#include "AppConstants.h"
#include "ImageUtil.h"
#include "DrawUtil.h"
#include "AppUtil.h"
#include <commctrl.h>
#include <Shlwapi.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>

namespace MainWindow
{
    const wchar_t* g_szClassName = L"HexViewWindowClass";
    const int WM_DISPLAY_HEX_DATA = WM_USER + 1;
    const int WM_UPDATE_IMAGE_LIST = WM_USER + 2; // 自定义UI更新消息
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

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL RegisterWindowClass(HINSTANCE hInstance);
    void CreateStatusBar(HWND hwnd); // 创建状态栏
    void RefreshWindow(HWND hwnd);
    bool IsImageFile(const std::wstring& filePath);
    bool CompareImageFileByCreateTime(const ImageFileInfo& a, const ImageFileInfo& b);
    void ProcessFolder(const std::wstring& folderPath, HWND hwnd);
    void ExtractImageData(const std::wstring& folderPath);
    std::string getHexStrFromPixelList(const std::vector<std::vector<ImageUtil::PixelInfo>>& pixelList);
    
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
        DeleteFileW(L"app.log");
        
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

        ImageUtil::InitGdiplus();

        // 创建状态栏（窗口创建后立即创建）
        CreateStatusBar(gMainWindow);
        DragAcceptFiles(gMainWindow, TRUE); // 启用拖拽
        DrawUtil::InitDraw();

        ShowWindow(gMainWindow, nCmdShow);
        UpdateWindow(gMainWindow);
        
        std::string testImgPath = ".\\test\\0.png";
        std::vector<std::vector<ImageUtil::PixelInfo>> pixelList = ImageUtil::TraverseImagePixels(testImgPath);
        getHexStrFromPixelList(pixelList);

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
            case WM_CREATE:
                break;

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
                    wsprintf(countText, L"图片数量：%d", imageCount);
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
                // 窗口销毁：等待线程结束，避免资源泄漏
                if (gWorkerThread && gWorkerThread->joinable()) {
                    gWorkerThread->join();
                }
                PostQuitMessage(0);
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
                    AppUtil::SaveLog(fullPath);
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

        AppUtil::SaveLog("gImageFiles.size() ", gImageFiles.size());
        ExtractImageData(folderPath);

        // 重置处理状态（原子操作，线程安全）
        gIsProcessing = false;
        AppUtil::SaveLog("ProcessFolder finish");
    }

    void ExtractImageData(const std::wstring& folderPath){
        if(gImageFiles.size() < 1) return;
        std::string dstFileName;
        bool parseFileName = true;
        for(auto& fileInfo : MainWindow::gImageFiles){
            AppUtil::SaveLog(fileInfo.filePath);
            std::string imagePath = AppUtil::Utf16ToUtf8(fileInfo.filePath);
            std::vector<std::vector<ImageUtil::PixelInfo>> pixelList = ImageUtil::TraverseImagePixels(imagePath);
            AppUtil::SaveLog("pixelList.size() ", pixelList.size());
            if(parseFileName){
                parseFileName = false;
                dstFileName = getHexStrFromPixelList(pixelList);
            }else{
                std::string hexStr = getHexStrFromPixelList(pixelList);

            }
        }

    }

    std::string getHexStrFromPixelList(const std::vector<std::vector<ImageUtil::PixelInfo>>& pixelList){
        std::string hexStr;
        size_t rowSize = pixelList.size();
        if(rowSize < 10) return hexStr;

        size_t maxCol = GetMaxColumnCount(pixelList);
        int blackTotal = 0;
        int aColorTotal = 0;
        int bColorTotal = 0;
        int cColorTotal = 0;

        bool findFirstGrid = false;     // 是否找到了第一个格子
        int rgbTotalForFirstGrid = 0;
        
        for (size_t col = 0; col < maxCol; ++col) {
            blackTotal = 0;
            aColorTotal = 0;
            bColorTotal = 0;
            cColorTotal = 0;

            for (size_t row = 0; row < pixelList.size(); ++row) {
                const auto& rowPixels = pixelList[row];
                if (col >= rowPixels.size()) {
                    continue;
                }

                const ImageUtil::PixelInfo& pixel = rowPixels[col];
                if(pixel.color == AppConst::COLOR_BLACK){
                    blackTotal += 1;
                }

                if(blackTotal >= AppConst::GRID_SIZE && col >= AppConst::GRID_SIZE){
                    if(pixel.color == AppConst::GRID_COLOR_A){
                        aColorTotal += 1;
                    }else if(pixel.color == AppConst::GRID_COLOR_B){
                        bColorTotal += 1;
                    }else if(pixel.color == AppConst::GRID_COLOR_C){
                        cColorTotal += 1;
                    }
                }
            }

            // 颜色数量至少是一个格子的大小才行
            if(aColorTotal < AppConst::GRID_SIZE) aColorTotal = 0;
            if(bColorTotal < AppConst::GRID_SIZE) bColorTotal = 0;
            if(cColorTotal < AppConst::GRID_SIZE) cColorTotal = 0;

            if(!findFirstGrid && aColorTotal == 0 && bColorTotal == 0 && cColorTotal == 0){
                rgbTotalForFirstGrid += 1;
            }
            if(rgbTotalForFirstGrid >= AppConst::GRID_SIZE){
                if(aColorTotal >= AppConst::GRID_SIZE || 
                   bColorTotal >= AppConst::GRID_SIZE || 
                   bColorTotal >= AppConst::GRID_SIZE){
                    findFirstGrid = true;
                }
            }

            AppUtil::SaveLog("col ", col
                , " colorTotal ", blackTotal
                , " ", aColorTotal
                , " ", bColorTotal
                , " ", cColorTotal
                , " ", rgbTotalForFirstGrid
                , " ", findFirstGrid
            );
        }


        return hexStr;
    }


} // namespace MainWindow end