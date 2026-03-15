#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace AppUtil
{
    bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData);
    void NextPage();
    void ReStart();
    
    // 读指定文件转换为16进制字符串
	// 示例：输入 main.data → 输出 "12AB05"（无分隔符）
    std::string FileToHexString(const std::string& filePath);

    // 将二进制数组（vector<unsigned char>）转换为16进制字符串
	// 示例：输入 {0x12, 0xAB, 0x05} → 输出 "12AB05"（无分隔符）
    std::string BinaryToHexString(const std::vector<unsigned char>& binVector);

    // 把指定字符串转换为16进制字符串
    // 示例：输入 A → 输出 "41"（无分隔符）
    std::string StringToHexString(const std::string& data);


    std::wstring Utf8ToUtf16(const std::string& utf8Str);
    std::string Utf16ToUtf8(const std::wstring& utf16Str);

    void UpdateStatusBarText(HWND statusBar, int partIndex, const wchar_t* text); // 更新状态栏某列文本
    void UpdateStatusBarText(HWND statusBar, int partIndex, const std::string& text);


} // namespace AppUtil end

