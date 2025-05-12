/*
   @Time     : 2018/7/9 10:12
   @Author   : 王一冰
   @Describe :
   @Software : Copyright (c) 2018 中国手游娱乐集团有限公司
*/
#pragma once
#include <cstddef>
#include <atomic>
#include <array>

namespace MahjongMemoryPool
{
    // 内存对齐大小为8字节
    constexpr size_t ALIGNMENT = 8;
    // 内存池支持的最大内存块大小为256KB
    constexpr size_t MAX_BYTES = 256 * 1024;
    // 空闲链表数组大小（最大字节数/对齐单位）
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;
    // 设定自由链表的大小
    static const size_t SystemThreshold = 64;
    static const size_t MAXBATCHSIZE = 4 * 1024; // 4kb

    // 内存块头部信息结构体
    struct BlockHeader
    {
        size_t m_nsize;      // 当前内存块的大小（单位字节）
        bool m_bInUse;       // 标记该内存块是否被使用
        BlockHeader* m_pstNext; // 指向下一个内存块头的指针（用于链表连接）
    };

    // 内存尺寸处理工具类
    class MahJongSizeClass
    {
    public:
        // 内存对齐处理函数：将输入字节数向上对齐到ALIGNMENT的倍数
        static size_t RoundUp(size_t bytes)
        {
            // 使用位运算实现高效对齐计算：(bytes + 7) & ~7
            return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        // 获取对应内存块尺寸在空闲链表中的索引
        static size_t GetIndex(size_t bytes)
        {
            // 确保处理的最小字节数不小于对齐单位（8字节）
            bytes = std::max(bytes, ALIGNMENT);
            // 计算公式：(对齐后的字节数 / 对齐单位) - 1
            // 示例：8字节返回0，16字节返回1，以此类推
            return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
        }
    };
}
