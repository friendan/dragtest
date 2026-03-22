#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
#cmake --build build --config Debug
#cmake --build build --config Release
cmake_minimum_required(VERSION 3.20)
project(dragtest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===================== 设置静态链接 CRT =====================
if(MSVC)
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} CONF_UPPER)
        if(CONF_UPPER STREQUAL "DEBUG")
            set(CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}} /MTd")
        else()
            set(CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}} /MT")
        endif()
        # 移除动态 CRT 的选项（防止冲突）
        string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}}")
        string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_${CONF_UPPER} "${CMAKE_CXX_FLAGS_${CONF_UPPER}}")
    endforeach()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

remove_definitions(-D_MBCS)
add_definitions(-DUNICODE -D_UNICODE)

# 强制MSVC编译器用UTF-8解析源码
add_compile_options(/utf-8)

#file(GLOB SRCS "*.h" "*.cpp")
set(MainAppSrcList
	AppConstants.h
	MainWindow.h
	MainWindow.cpp
	DrawUtil.h
	DrawUtil.cpp
	AppUtil.h
	AppUtil.cpp
	ImageUtil.h
	ImageUtil.cpp
	main.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MainAppSrcList})
add_executable(MainApp WIN32 ${MainAppSrcList})
target_link_libraries(MainApp PRIVATE shell32.lib comctl32.lib shlwapi.lib gdiplus.lib)


#=================================================
set(SlaveAppSrcList
	AppConstants.h
	SlaveWindow.h
	SlaveWindow.cpp
	DrawUtil.h
	DrawUtil.cpp
	AppUtil.h
	AppUtil.cpp
	secondary.cpp
	GridMap.rc
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SlaveAppSrcList})
add_executable(GridMap WIN32 ${SlaveAppSrcList})
target_link_libraries(GridMap PRIVATE shell32.lib comctl32.lib shlwapi.lib)






