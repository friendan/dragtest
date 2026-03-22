#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace AppUtil
{
    bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData);
    std::string ReadFileHexString(const std::wstring& filePath);
    
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

    std::string GetTimeStr();
    void SaveLog(const std::string& msg);
    void SaveLog(const std::wstring& msg);

    // template <typename T>
    // void SaveLog(const T& value) {
    //     std::stringstream ss;
    //     ss << value;
    //     SaveLog(ss.str());
    // }

    // 变参模板：支持任意数量、任意类型的参数
    template <typename... Args>
    void SaveLog(const Args&... args) {
        std::stringstream ss;
        // 折叠表达式：依次将所有参数写入stringstream（C++17+）
        (ss << ... << args);
        // 调用基础重载保存日志
        SaveLog(ss.str());
    }

    // 宽字符变参模板（显式指定宽字符参数）
    // 模板参数后缀W，明确区分宽字符版本
    template <typename... Args>
    void SaveLogW(const Args&... args) {
        std::wstringstream wss; // 改用宽字符流
        (wss << ... << args);   // 折叠表达式支持wstring/int等
        SaveLog(wss.str());     // 调用宽字符基础重载
    }

    // 十六进制字符串转原始字符串（无流操作，高性能）
    std::string hexStrToStr(const std::string& hexStr);

    bool hexStrToFile(const std::string& hexStr, const std::string& path);

    std::string GetRandHexString(size_t len);
    size_t GetRandNumber(size_t min, size_t max);

} // namespace AppUtil end

