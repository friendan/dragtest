#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <gdiplus.h>

namespace ImageUtil
{
    struct PixelInfo {
        BYTE red;       // 红色分量 (0-255)
        BYTE green;     // 绿色分量 (0-255)
        BYTE blue;      // 蓝色分量 (0-255)
        BYTE alpha;     // 透明通道 (0-255)
        COLORREF color; 
    };

    bool InitGdiplus();
    void ShutdownGdiplus();
   
    /**
     * @brief 遍历图片所有像素，返回二维结构（行→列）
     * @param imagePath 图片路径（普通字符串）
     * @return std::vector<std::vector<PixelInfo>> 二维像素数据：[行][列]，失败返回空
     */
    std::vector<std::vector<PixelInfo>> TraverseImagePixels(const std::string& imagePath);

    // 计算二维vector的最大列数（所有行中最长的列数）
    size_t GetMaxColumnCount(const std::vector<std::vector<ImageUtil::PixelInfo>>& pixelList);


} // namespace ImageUtil end

