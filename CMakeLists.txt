#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
#cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
#cmake --build build --config Debug
#cmake --build build --config Release
cmake_minimum_required(VERSION 3.20)
project(dragtest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

remove_definitions(-D_MBCS)
add_definitions(-DUNICODE -D_UNICODE)

# 强制MSVC编译器用UTF-8解析源码
add_compile_options(/utf-8)

file(GLOB SRCS "*.h" "*.cpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})
#add_executable(${PROJECT_NAME} ${SRCS})
add_executable(${PROJECT_NAME} WIN32 ${SRCS})

# 链接拖拽功能必需的shell32.lib
target_link_libraries(${PROJECT_NAME} PRIVATE shell32.lib)








