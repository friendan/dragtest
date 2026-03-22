#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include "AppConstants.h"
#include "AppUtil.h"

namespace DrawUtil
{
    HBRUSH gBrushList[3]; // 3个画刷对应三进制的3位
    size_t gTotalPage = 0;
    size_t gCurrentPage = 0;
    size_t gPageCharNum = 0;
    size_t gGridSizeAdd = 0;
    HWND gDrawWindow = NULL;
    std::string gNameHexStr;

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector);

    void InitDraw(){
        gBrushList[0] = CreateSolidBrush(AppConst::GRID_COLOR_A);
        gBrushList[1] = CreateSolidBrush(AppConst::GRID_COLOR_B);
        gBrushList[2] = CreateSolidBrush(AppConst::GRID_COLOR_C);
    }

    void UnInitDraw(){

    }

    void NextPage(){
        if(gCurrentPage < gTotalPage){
            gCurrentPage += 1;
        }
        else if(gCurrentPage == 0){
            gCurrentPage = 1;
        }
    }

    void ReStart(){
        gCurrentPage = 0;
        gTotalPage = 0;
        gGridSizeAdd = 0;
    }

    bool AddGridSize(int addVal){
        size_t gGridSizeAddOld = gGridSizeAdd;
        gGridSizeAdd += addVal;
        
        std::vector<RECT> rectVector;
        GetWindowGridVector(gDrawWindow, rectVector);
        if(rectVector.size() < gNameHexStr.size()*3){
            gGridSizeAdd = gGridSizeAddOld;
            return false;
        }
        return true;
    }

    bool DecGridSize(int decVal){
        // gGridSizeAdd -= decVal; // 不能这样 size_t是无符号整数 0-=1会溢出，导致程序异常
        if(gGridSizeAdd <= 0){
            return false;
        }
        if(decVal >= gGridSizeAdd){
            gGridSizeAdd = 0;
        }else{
            gGridSizeAdd -= decVal;
        }
        return true;
    }
    
    size_t GetGridSize(){
        return AppConst::GRID_SIZE + gGridSizeAdd;
    }

    size_t GetNowPage(){
        return gCurrentPage;
    }

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector){
        gDrawWindow = hwnd;
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        size_t gridSize = GetGridSize();
        int width  = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;
        size_t borderDistance = 8;
        if(gridSize > borderDistance){
            borderDistance = gridSize;
        }
        int startX = borderDistance*2;
        int startY = borderDistance*2;
        int xMax = width  - borderDistance*3;
        int yMax = height - borderDistance*5;

        int xOffset = 0;
        for(int x = startX; x < xMax; x += gridSize){
            int yOffset = 0;
            for(int y = startY; y < yMax; y += gridSize){
                RECT rect;
                rect.left   = x + xOffset; 
                rect.top    = y + yOffset;
                rect.right  = x + gridSize + xOffset;
                rect.bottom = y + gridSize + yOffset;
                if(rect.right >= xMax || rect.bottom >= yMax){
                    break;
                }
                rectVector.push_back(rect);
                yOffset += AppConst::GRID_OFFSET_Y;
            }
            xOffset += AppConst::GRID_OFFSET_X;
        }
    }

    void DrawFileNameGrid(HWND hwnd, const std::string& fileNameHexStr){
        if(fileNameHexStr.size() < 1){
            return;
        }
        std::vector<RECT> rectVector;
        GetWindowGridVector(hwnd, rectVector);
        if(rectVector.size() < fileNameHexStr.size()*3){
            return; // 当前窗口格子数太少了，无法绘制文件名
        }
        HDC hdc = GetDC(hwnd);
        size_t rectIndex = 0;
        for(char hexChar: fileNameHexStr){
            int charInt = AppConst::CHAR_TO_DEC_MAP[hexChar];
            const int* brushIndexs = AppConst::BRUSH_TRIAD_TABLE[charInt];
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[0]]);
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[1]]);
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[2]]);
        }
        ReleaseDC(hwnd, hdc);
    }

    void DrawHexStringGrid(HWND hwnd, const std::string& hexStr){
        std::vector<RECT> rectVector;
        GetWindowGridVector(hwnd, rectVector);
        if(rectVector.size() < 3){
            return; // 三进制 至少要3个格子才行
        }

        gPageCharNum = rectVector.size() / 3;
        if(gPageCharNum < 1){
            gPageCharNum = 1; // 不能是0 不然后面除法 程序崩溃
        }
        gTotalPage = hexStr.size() / gPageCharNum;
        if(hexStr.size() % gPageCharNum != 0){
            gTotalPage = gTotalPage + 1;
        }
        if(gTotalPage < 1){
            gTotalPage = 1;
            gCurrentPage = 1;
        }

        HDC hdc = GetDC(hwnd);
        size_t gridSize = GetGridSize();
        size_t charIndex = 0;
        size_t maxIndex = hexStr.size() - 1;
        size_t nowPage = gCurrentPage - 1;
        size_t rectIndex = 0;
        for(size_t cnt = 0; cnt < gPageCharNum; cnt++){
            charIndex = nowPage*gPageCharNum + cnt;
            if(charIndex > maxIndex){
                break;
            }
            char hexChar = hexStr[charIndex];
            int charInt = AppConst::CHAR_TO_DEC_MAP[hexChar];
            const int* brushIndexs = AppConst::BRUSH_TRIAD_TABLE[charInt];
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[0]]);
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[1]]);
            FillRect(hdc, &rectVector[rectIndex++], gBrushList[brushIndexs[2]]);

            /*
            RECT& rect = rectVector[rectIndex-1];
            size_t x1 = 0;
            size_t y1 = rect.top + gridSize;
            size_t x2 = width;
            size_t y2 = y1;
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // PS_SOLID=实线，1=宽度，RGB=颜色
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen); // 将自定义画笔选入DC，保存旧画笔
            MoveToEx(hdc, x1, y1, NULL); 
            LineTo(hdc, x2, y2);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            */
        }
        ReleaseDC(hwnd, hdc);
    }

    void ShowDrawDataInfo(HWND hwnd, HWND statusBar, const std::string& hexStr){
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int width  = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;
        std::vector<RECT> rectVector;
        GetWindowGridVector(hwnd, rectVector);

        WCHAR szTitle[1024] = {0};
        wsprintf(szTitle, L"%s %dx%d|%d"
            , AppConst::SLAVE_APP_TITLE
            , width
            , height
            , rectVector.size()
        );
        SetWindowTextW(hwnd, szTitle);
        
        WCHAR szPageInfo[64] = {0};
        wsprintf(szPageInfo, L"%d|%d", gTotalPage, gCurrentPage);
        AppUtil::UpdateStatusBarText(statusBar, 0, szPageInfo);

        WCHAR szCharInfo[64] = {0};
        wsprintf(szCharInfo, L"%d", hexStr.size());
        AppUtil::UpdateStatusBarText(statusBar, 1, szCharInfo);
        
        WCHAR szPageChar[1024] = {0};
        wsprintf(szPageChar, L"%d"
            , gPageCharNum
        );
        AppUtil::UpdateStatusBarText(statusBar, 2, szPageChar);
    }

    void DrawBackground(HWND hwnd){
        HDC hdc = GetDC(hwnd);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        ReleaseDC(hwnd, hdc);
    }

    void DrawDataGrid(HWND hwnd, HWND statusBar, const std::string& fileNameHexStr, const std::string& hexStr)
    {
        gNameHexStr = fileNameHexStr;
        DrawBackground(hwnd);
        if(gCurrentPage <= 0){
            DrawFileNameGrid(hwnd, fileNameHexStr);
            ShowDrawDataInfo(hwnd, statusBar, fileNameHexStr);
        }else{
            DrawHexStringGrid(hwnd, hexStr);
            ShowDrawDataInfo(hwnd, statusBar, hexStr);
        }
    }
    


} // namespace DrawUtil end

