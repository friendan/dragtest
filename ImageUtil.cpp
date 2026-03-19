#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include "ImageUtil.h"

namespace ImageUtil
{
	static ULONG_PTR g_gdiplusToken = 0;
	static UINT g_imageWidth = 0;
    static UINT g_imageHeight = 0;

	bool InitGdiplus() {
        if (g_gdiplusToken != 0) {
            return true;
        }
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::Status status = Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
        if (status != Gdiplus::Ok) {
            return false;
        }
        return true;
    }

    void ShutdownGdiplus() {
        if (g_gdiplusToken == 0) {
            return;
        }
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0; 
    }
	
    std::vector<std::vector<PixelInfo>> TraverseImagePixels(const std::string& imagePath) {
        std::vector<std::vector<PixelInfo>> pixel2DData;

        // 普通字符串转宽字符（适配GDI+）
        int wlen = MultiByteToWideChar(CP_ACP, 0, imagePath.c_str(), -1, NULL, 0);
        wchar_t* wpath = new wchar_t[wlen];
        MultiByteToWideChar(CP_ACP, 0, imagePath.c_str(), -1, wpath, wlen);

        // 加载图片
        Gdiplus::Bitmap bitmap(wpath);
        delete[] wpath; // 释放临时宽字符内存

        if (bitmap.GetLastStatus() != Gdiplus::Ok) {
            return pixel2DData;
        }

        // 获取图片尺寸并缓存
        g_imageWidth = bitmap.GetWidth();
        g_imageHeight = bitmap.GetHeight();

        // 初始化二维vector的行数（高度）
        pixel2DData.resize(g_imageHeight);

        // 为每一行初始化列数（宽度），避免后续push_back扩容
        for (UINT y = 0; y < g_imageHeight; y++) {
            pixel2DData[y].resize(g_imageWidth);
        }

        // 锁定像素缓冲区（高效遍历）
        Gdiplus::BitmapData bitmapData;
        Gdiplus::Rect rect(0, 0, g_imageWidth, g_imageHeight);
        Gdiplus::Status status = bitmap.LockBits(
            &rect,
            Gdiplus::ImageLockModeRead,
            Gdiplus::PixelFormat32bppARGB,
            &bitmapData
        );

        if (status != Gdiplus::Ok) {
            // std::cerr << "错误：锁定像素缓冲区失败！错误码：" << status << std::endl;
            return pixel2DData;
        }

        // 遍历所有像素，填充二维结构
        UINT stride = bitmapData.Stride;
        BYTE* pPixels = (BYTE*)bitmapData.Scan0;

        for (UINT y = 0; y < g_imageHeight; y++) { // 遍历每一行
            BYTE* pRow = pPixels + y * stride;
            for (UINT x = 0; x < g_imageWidth; x++) { // 遍历该行的每一列
                // 提取颜色分量（字节顺序：B G R A）
                BYTE blue = pRow[x * 4];
                BYTE green = pRow[x * 4 + 1];
                BYTE red = pRow[x * 4 + 2];
                BYTE alpha = pRow[x * 4 + 3];

                // 直接赋值到二维结构：第y行第x列
                pixel2DData[y][x] = { red, green, blue, alpha };
            }
        }

        // 解锁缓冲区
        bitmap.UnlockBits(&bitmapData);
        return pixel2DData;
    }



} // namespace ImageUtil end

