#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace DrawUtil
{   
    //参数：hexStr 必须是16进制字符串
    void DrawDataGrid(HWND hwnd, HWND statusBar, const std::string& hexStr);

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector);







} // namespace DrawUtil end

