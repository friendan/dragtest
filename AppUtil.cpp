#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iomanip>  // 用于设置16进制输出格式
#include <commctrl.h>
#include <mutex>
#include <ctime>
#include <cctype> // 用于toupper
#include <random>

namespace AppUtil
{
	static std::mutex g_log_mutex;
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

    std::string GetTimeStr(){
    	time_t now = time(nullptr);
	    tm t{};
	    localtime_s(&t, &now); // Windows下安全的本地时间函数
	    char buf[32] = {0};
	    sprintf_s(buf, "[%04d-%02d-%02d %02d:%02d:%02d]", 
	             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
	             t.tm_hour, t.tm_min, t.tm_sec);
	    return buf;
    }

    void write_log(const std::string& msg) {
	    std::lock_guard<std::mutex> lock(g_log_mutex); // 自动加锁/解锁
	    std::ofstream log_file("app.log", std::ios::app | std::ios::out);
	    if (log_file.is_open()) {
	        log_file << GetTimeStr() << " " << msg << std::endl;
	        log_file.close();
	    }
	}

	void SaveLog(const std::string& msg){
		write_log(msg);
	}

	void SaveLog(const std::wstring& msg){
		write_log(Utf16ToUtf8(msg));
	}

	// 辅助函数：单个十六进制字符转数值（0-15），非法字符返回-1
	int hexCharToVal(char c) {
	    c = static_cast<char>(std::toupper(c));  // 大写转换，兼容小写输入
	    if (c >= '0' && c <= '9') {
	        return c - '0'; // 0-9 → 0-9
	    } else if (c >= 'A' && c <= 'F') {
	        return c - 'A' + 10; // A-F → 10-15
	    }
	    return -1; // 非法字符
	}

	// 高性能：2个十六进制字符转1个字节
	inline unsigned char hexPairToByte(const std::string& hexStr, size_t pos) {
	    unsigned char high = hexCharToVal(hexStr[pos]);
	    unsigned char low  = hexCharToVal(hexStr[pos + 1]);
	    // 高位左移4位 + 低位（如 "1A" → 0x10 + 0xA = 0x1A）
	    return (high << 4) | low;
	}

	std::string hexStrToStr(const std::string& hexStr) {
	    std::string result;
	    result.reserve(hexStr.length() / 2); // 预分配内存，避免多次扩容
	    
	    // 校验长度（偶数）
	    if (hexStr.length() % 2 != 0) {
	        // throw std::invalid_argument("十六进制字符串长度必须为偶数");
	        return result;
	    }

	    // 手动解析每两位
	    for (size_t i = 0; i < hexStr.length(); i += 2) {
	        char c1 = hexStr[i];
	        char c2 = hexStr[i+1];
	        int val1 = hexCharToVal(c1);
	        int val2 = hexCharToVal(c2);
	        if (val1 == -1 || val2 == -1) {
	        	return result;
	        }
	        // 组合为字节（高位<<4 + 低位，等价于 val1*16 + val2）
	        unsigned char byte = static_cast<unsigned char>((val1 << 4) | val2);
	        result.push_back(byte);
	    }
	    return result;
	}

	bool hexStrToFile(const std::string& hexStr, const std::string& path){
	    if (hexStr.empty()) {
	        return false;
	    }
	    if (hexStr.length() % 2 != 0) {
	    	return false; // 十六进制字符串长度必须为偶数（每2个字符对应1个字节）
	    }

	    std::ofstream file(path, std::ios::out | std::ios::app | std::ios::binary);
	    if (!file.is_open()) {
	      	return false;
	    }

	    for (size_t i = 0; i < hexStr.length(); i += 2) {
	        unsigned char byte = hexPairToByte(hexStr, i);
            file.write(reinterpret_cast<const char*>(&byte), 1);
	    }
	    file.close();
	    return true;
	}

	std::string GetRandHexString(size_t len){
	    if (len == 0) {
	       return "";
	    }
	    static char hex_chars[] = "0123456789ABCDEF";

	    // 初始化随机数生成器（C++11 及以上推荐的安全方式）
	    // 静态变量：避免每次调用重复初始化，提升性能且保证随机性
	    static std::random_device rd;  // 硬件随机数种子（非确定性）
	    static std::mt19937 gen(rd()); // 梅森旋转算法，高性能伪随机数生成器
	    static std::uniform_int_distribution<> dist(0, 15); // 0-15 的均匀分布

	    std::string hex_str;
	    hex_str.reserve(len);
	    for (size_t i = 0; i < len; ++i) {
	        hex_str += hex_chars[dist(gen)];
	    }
	    return hex_str;
	}

	size_t GetRandNumber(size_t min, size_t max){
	    if (min > max) {
	   		return 0;
	    }
	    if (min == max) {
	        return min;
	    }

	    // 初始化随机数生成器（静态变量：仅初始化一次，提升性能）
	    // std::random_device：硬件随机数种子（非确定性，更安全）
	    static std::random_device rd;
	    // std::mt19937：高性能梅森旋转伪随机数生成器（优于rand()）
	    static std::mt19937 gen(rd());

	    //  定义闭区间 [min, max] 的均匀分布
	    // 注意：std::uniform_int_distribution 支持 size_t 类型（C++11及以上）
	    std::uniform_int_distribution<size_t> dist(min, max);
	    return dist(gen);
	}
	

} // namespace AppUtil end

