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
    bool AddGridSize(int addVal);
    bool DecGridSize(int decVal);
    size_t GetGridSize();
    size_t GetNowPage();

    //参数：hexStr 必须是16进制字符串
    void DrawDataGrid(HWND hwnd, HWND statusBar, const std::string& fileNameHexStr, const std::string& hexStr);

    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector);
    void DrawFileNameGrid(HWND hwnd, const std::string& fileNameHexStr);
    void DrawHexStringGrid(HWND hwnd, const std::string& hexStr);
    void ShowDrawDataInfo(HWND hwnd, HWND statusBar, const std::string& hexStr);
    void DrawBackground(HWND hwnd);




} // namespace DrawUtil end

