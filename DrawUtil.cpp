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
    size_t gTotalPage = 1;   // min is 1
    size_t gCurrentPage = 1; // min is 1
    size_t gPageCharNum = 0;
    size_t gGridSizeAdd = 0;
    HWND gDrawWindow = NULL;

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
    }

    void ReStart(){
        gCurrentPage = 1;
    }

    void AddGridSize(int addVal){
        gGridSizeAdd += addVal;
        if(gDrawWindow != NULL){
            RECT rcClient;
            GetClientRect(gDrawWindow, &rcClient);
            int width  = (rcClient.right - rcClient.left) / 5;
            int height = (rcClient.bottom - rcClient.top) / 5;
            if(gGridSizeAdd > width || gGridSizeAdd > height){
                gGridSizeAdd = width;
            }
        }
    }

    void DecGridSize(int decVal){
        // gGridSizeAdd -= decVal; // 不能这样 size_t是无符号整数 0-=1会溢出，导致程序异常
        if(decVal > gGridSizeAdd){
            gGridSizeAdd = 0;
        }else{
            gGridSizeAdd -= decVal;
        }
    }

    size_t GetGridSize(){
        return AppConst::GRID_SIZE + gGridSizeAdd;
    }

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector){
        gDrawWindow = hwnd;
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        size_t gridSize = GetGridSize();
        int width  = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;
        int startX = gridSize;
        int startY = gridSize;
        int xMax = width  - gridSize*3;
        int yMax = height - gridSize*3;

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

    void DrawDataGrid(HWND hwnd, HWND statusBar, const std::string& hexStr)
    {
        HDC hdc = GetDC(hwnd);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        int width  = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;

        std::vector<RECT> rectVector;
        GetWindowGridVector(hwnd, rectVector);

        gPageCharNum = rectVector.size() / 3;
        gTotalPage = hexStr.size() / gPageCharNum;
        if(hexStr.size() % gPageCharNum != 0){
            gTotalPage = gTotalPage + 1;
        }
        if(gTotalPage < 1){
            gTotalPage = 1;
            gCurrentPage = 1;
        }

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

      
        WCHAR szTitle[1024] = {0};
        wsprintf(szTitle, L"%s %d %d|%d %d"
            , AppConst::SLAVE_APP_TITLE
            , width
            , height
            , rectVector.size()
            , rectVector.size() / 3
        );
        SetWindowTextW(hwnd, szTitle);

        WCHAR szPageInfo[64] = {0};
        wsprintf(szPageInfo, L"%d|%d|%d", gPageCharNum, gTotalPage, gCurrentPage);
        AppUtil::UpdateStatusBarText(statusBar, 0, szPageInfo);

        WCHAR szCharInfo[64] = {0};
        wsprintf(szCharInfo, L"%d %d", hexStr.size(), hexStr.size()*3);
        AppUtil::UpdateStatusBarText(statusBar, 1, szCharInfo);

        // AppUtil::UpdateStatusBarText(statusBar, 2, hexStr);
        ReleaseDC(hwnd, hdc);
    }
    


} // namespace DrawUtil end

