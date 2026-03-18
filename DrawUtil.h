#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace DrawUtil
{   
    void InitDraw();
    void UnInitDraw();
    void NextPage();
    void ReStart();
    void AddGridSize(int addVal);
    void DecGridSize(int decVal);
    size_t GetGridSize();
    size_t GetNowPage();

    //参数：hexStr 必须是16进制字符串
    void DrawDataGrid(HWND hwnd, HWND statusBar, const std::string& hexStr);

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector);







} // namespace DrawUtil end

