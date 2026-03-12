#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>

namespace AppUtil
{
    bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData);
    
    // 读指定文件转换为16进制字符串
	// 示例：输入 main.data → 输出 "12AB05"（无分隔符）
    std::string FileToHexString(const std::string& filePath);

    // 将二进制数组（vector<unsigned char>）转换为16进制字符串
	// 示例：输入 {0x12, 0xAB, 0x05} → 输出 "12AB05"（无分隔符）
    std::string BinaryToHexString(const std::vector<unsigned char>& binVector);









} // namespace AppUtil end

