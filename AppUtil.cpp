#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iomanip>  // 用于设置16进制输出格式
#include <commctrl.h>

namespace AppUtil
{
	std::string StringToHexString(const std::string& data);
	
	bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData) {
	    outData.clear();
	    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	    if (!file.is_open()) return false;
	    std::streamsize size = file.tellg();
	    file.seekg(0, std::ios::beg);
	    outData.resize(size);
	    if (!file.read((char*)outData.data(), size)) {
	        outData.clear();
	        file.close();
	        return false;
	    }
	    file.close();
	    return true;
	}

	 std::string ReadFileHexString(const std::wstring& filePath){
	    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	    if (!file.is_open()) return "";
	    std::streamsize size = file.tellg();
	    file.seekg(0, std::ios::beg);
	    std::string data;
	    data.resize(size);
	   	file.read((char*)data.data(), size);
	   	file.close();
	    return StringToHexString(data);
	 }

	std::string BinaryToHexString(const std::vector<unsigned char>& binVector){
	    std::ostringstream oss;
	    oss << std::hex << std::uppercase << std::setfill('0');  // 设置输出格式：十六进制、大写、不足两位补0 小写可改用std::nouppercase
	    for (size_t i = 0; i < binVector.size(); ++i)
	    {
	        unsigned int byteValue = static_cast<unsigned int>(binVector[i]);  // 将unsigned char转为整数，确保按数值解析（而非ASCII字符）
	        oss << std::setw(2) << byteValue; // 格式化为两位16进制（std::setw(2)确保不足两位补0）
	        // if (i != binVector.size() - 1) oss << " "; // 可选：添加分隔符（如空格），方便阅读
	    }
	    return oss.str();
	}

	
	std::string FileToHexString(const std::string& filePath){
	    std::ifstream file(filePath, std::ios::binary | std::ios::in); // 以二进制只读模式打开文件
	    if (!file.is_open())
	    {
	      	return "";
	    }
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        if (fileSize <= 0)
        {
        	file.close();
            return "";
        }
        std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
        {
        	file.close();
            return "";
        }
        file.close();
        return BinaryToHexString(buffer);
	}
	
	std::string StringToHexString(const std::string& data){
	    std::ostringstream oss;
	    oss << std::hex << std::uppercase << std::setfill('0');
	    for (unsigned char c : data)  // 用unsigned char避免符号位问题（如0x80以上字符）
	    {
	        unsigned int charValue = static_cast<unsigned int>(c);  // 将char转为无符号整数，确保按ASCII值解析（而非字符本身）
	        oss << std::setw(2) << charValue;
	    }
	    return oss.str();
	}

	std::wstring Utf8ToUtf16(const std::string& utf8Str){
		if (utf8Str.empty())
	    {
	        return L"";
	    }
	    // 第一步：计算需要的宽字符缓冲区大小
	    int wideCharCount = MultiByteToWideChar(
	        CP_UTF8,        // 源编码：UTF-8
	        0,              // 标志：无特殊处理
	        utf8Str.c_str(),// 源UTF-8字符串
	        -1,             // 自动计算字符串长度（包含\0）
	        nullptr,        // 临时缓冲区：先传NULL计算大小
	        0               // 缓冲区大小：0表示仅计算
	    );
	    if (wideCharCount == 0)
	    {
	        return L""; // 转换失败返回空
	    }
	    // 第二步：分配缓冲区并执行转换
	    std::wstring utf16Str(wideCharCount, L'\0');
	    MultiByteToWideChar(
	        CP_UTF8,
	        0,
	        utf8Str.c_str(),
	        -1,
	        &utf16Str[0],
	        wideCharCount
	    );
	    // 移除自动添加的\0（可选，Win32 API可识别带\0的字符串）
	    utf16Str.pop_back();
	    return utf16Str;
	}

	std::string Utf16ToUtf8(const std::wstring& utf16Str){
		if (utf16Str.empty())
	    {
	        return "";
	    }
	    int multiByteCount = WideCharToMultiByte(
	        CP_UTF8,
	        0,
	        utf16Str.c_str(),
	        -1,
	        nullptr,
	        0,
	        nullptr,
	        nullptr
	    );
	    if (multiByteCount == 0)
	    {
	        return "";
	    }
	    std::string utf8Str(multiByteCount, '\0');
	    WideCharToMultiByte(
	        CP_UTF8,
	        0,
	        utf16Str.c_str(),
	        -1,
	        &utf8Str[0],
	        multiByteCount,
	        nullptr,
	        nullptr
	    );
	    utf8Str.pop_back();
	    return utf8Str;
	}

	// 更新状态栏指定列的文本
    void UpdateStatusBarText(HWND statusBar, int partIndex, const wchar_t* text) {
        // SB_SETTEXT：设置指定列文本；0：绘制方式（普通）
        SendMessageW(statusBar, SB_SETTEXT, partIndex, (LPARAM)text);
    }

    void UpdateStatusBarText(HWND statusBar, int partIndex, const std::string& text){
        std::wstring wideText = AppUtil::Utf8ToUtf16(text);
        SendMessageW(statusBar, SB_SETTEXT, partIndex, (LPARAM)wideText.c_str());
    }
	

} // namespace AppUtil end

