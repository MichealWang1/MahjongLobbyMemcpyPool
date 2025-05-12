# CMakeList.txt: MahjongLobbyMemcpyPool 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("MahjongLobbyMemcpyPool")

# 将源代码添加到此项目的可执行文件。
add_executable (MahjongLobbyMemcpyPool "MahjongLobbyMemcpyPool.cpp" "MahjongLobbyMemcpyPool.h"   "common/mahjongthreadcache.h" "common/mahjongthreadcache.cpp" "common/common.h" "common/majhongcentralcache.h" "common/majhongcentralcache.cpp" "common/majhongpagecache.h" "common/majhongpagecache.cpp" "common/majhongmemorypool.h")

# TODO: 如有需要，请添加测试并安装目标。
