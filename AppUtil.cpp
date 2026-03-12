#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iomanip>  // 用于设置16进制输出格式

namespace AppUtil
{
	bool ReadFileToBinary(const std::wstring& filePath, std::vector<unsigned char>& outData) {
	    outData.clear();
	    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	    if (!file.is_open()) return false;
	    std::streamsize size = file.tellg();
	    file.seekg(0, std::ios::beg);
	    outData.resize(size);
	    if (!file.read((char*)outData.data(), size)) {
	        outData.clear();
	        return false;
	    }
	    return true;
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
	

} // namespace AppUtil end

