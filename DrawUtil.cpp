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

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector){
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        int width  = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;
        int startX = AppConst::GRID_SIZE;
        int startY = AppConst::GRID_SIZE;
        int xMax = width  - AppConst::GRID_SIZE - AppConst::GRID_SIZE;
        int yMax = height - AppConst::GRID_SIZE - AppConst::GRID_SIZE;

        int xOffset = 0;
        for(int x = startX; x < xMax; x += AppConst::GRID_SIZE){
            int yOffset = 0;
            for(int y = startY; y < yMax; y += AppConst::GRID_SIZE){
                RECT rect;
                rect.left   = x + xOffset; 
                rect.top    = y + yOffset;
                rect.right  = x + AppConst::GRID_SIZE + xOffset;
                rect.bottom = y + AppConst::GRID_SIZE + yOffset;
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

            RECT& rect = rectVector[rectIndex-1];
            size_t x1 = 0;
            size_t y1 = rect.top + AppConst::GRID_SIZE;
            size_t x2 = width;
            size_t y2 = y1;
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // PS_SOLID=实线，1=宽度，RGB=颜色
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen); // 将自定义画笔选入DC，保存旧画笔
            MoveToEx(hdc, x1, y1, NULL); 
            LineTo(hdc, x2, y2);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
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

        AppUtil::UpdateStatusBarText(statusBar, 2, hexStr);

        ReleaseDC(hwnd, hdc);
    }



    void DrawGrid(HWND hwnd) {
        // HDC hdc = GetDC(hwnd);
        // RECT rcClient;
        // GetClientRect(hwnd, &rcClient);
        // FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
        // int width  = rcClient.right - rcClient.left;
        // int height = rcClient.bottom - rcClient.top;
        // int startX = 16;
        // int startY = 16;
        // int xMax = width - GRID_SIZE;
        // int yMax = height - GRID_SIZE*2;

        // int color = 0;
        // RECT rect;
        // int colCount = 0;
        // int xOffset = 0;
        // int gridNum = 0;
        // for(int x = startX; x < xMax; x += GRID_SIZE){
        //     for(int y = startY; y < yMax; y += GRID_SIZE){
        //         rect.left   = x + xOffset; 
        //         rect.top    = y;
        //         rect.right  = x + GRID_SIZE + xOffset;
        //         rect.bottom = y + GRID_SIZE;
        //         if(rect.right > xMax){
        //             break;
        //         }
        //         FillRect(hdc, &rect, gBrushList[color%3]);
        //         color = color + 1;
        //         gridNum = gridNum + 1;
        //     }
        //     colCount = colCount + 1;
        //     xOffset = xOffset + 1;
        //     if(colCount >= 6){
        //         // break;
        //     }
        // }

        // WCHAR szTitle[256] = {0};
        // wsprintf(szTitle, L"Console：%d %d|%d|%d %d|", width, height, colCount, gridNum, gridNum/3);
        // SetWindowText(hwnd, szTitle);
       
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


        // ReleaseDC(hwnd, hdc);
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
        // HFONT hOldFont = NULL;
        // if (g_hConsoleFont) {
        //     hOldFont = (HFONT)SelectObject(hdc, g_hConsoleFont);
        // }

        // 4. 生成16进制显示文本
        // std::wstring hexText = BinaryToHex(g_fileData, g_currentPage);

        // 5. 自适应绘制（支持自动换行，确保翻页后内容适配窗口）
        int offset = 20;
        RECT rcText = rcClient;
        rcText.left += offset;
        rcText.top += offset;
        rcText.right -= offset;
        rcText.bottom -= offset;

        // DT_EDITCONTROL+DT_WORDBREAK：任意字符换行；DT_LEFT：左对齐；DT_NOCLIP：不裁剪
        // UINT drawFlags = DT_WORDBREAK | DT_LEFT | DT_NOCLIP | DT_EDITCONTROL;
        // DrawText(hdc, hexText.c_str(), -1, &rcText, drawFlags);

        // 6. 释放资源
        // if (hOldFont) SelectObject(hdc, hOldFont);
        // ReleaseDC(hwnd, hdc);
    }







} // namespace DrawUtil end

