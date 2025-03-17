cmake_minimum_required(VERSION 2.8)# 需求的最低的cmake程序版本
project(ZFWTinyMuduo)

include_directories(base)

#ZFWTinyMuduo最终编译成.so动态库，设置动态库路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置调试信息 以及 启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义参与编译的源代码文件 .指的是该项目根目录下所有源文件
# aux_source_directory(. SRC_LIST)

file(GLOB SRC_LIST "base/*.cc" "net/*.cc")
#编译生成动态库ZFWTinyMuduo
add_library(ZFWTinyMuduo SHARED ${SRC_LIST})