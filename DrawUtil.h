#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace DrawUtil
{
    void DrawDataGrid(HWND hwnd, const std::string& data);
    void GetWindowGridVector(HWND hwnd, std::vector<RECT>& rectVector);







} // namespace DrawUtil end

