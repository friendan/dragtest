#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace AppConst
{
    constexpr int GRID_SIZE = 16;
    constexpr int GRID_OFFSET_X = 5;
    constexpr int GRID_OFFSET_Y = 5;
    constexpr COLORREF GRID_COLOR_A = RGB(255, 255, 255);
    constexpr COLORREF GRID_COLOR_B = RGB(255, 255, 0);
    constexpr COLORREF GRID_COLOR_C = RGB(0, 255, 255);
    constexpr const wchar_t* SLAVE_APP_TITLE = L"GridMap";
    



    //std::string utf8_str = u8"中文测试😀"; // C++11+支持u8前缀标识UTF-8字面量
    




} // namespace Win32App end

