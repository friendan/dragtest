#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace SlaveWindow
{
    void StartMainWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    void UpdateStatusBarText(int partIndex, const wchar_t* text); // 更新状态栏某列文本

} // namespace SlaveWindow end










