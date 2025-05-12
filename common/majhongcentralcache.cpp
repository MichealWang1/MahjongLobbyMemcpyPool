#include <cassert>
#include <thread>
#include "majhongcentralcache.h"
#include "majhongpagecache.h"


namespace MahjongMemoryPool 
{
    // 每个页缓存大小（单位：页）
    static const size_t PAGECACHESIZE = 8;

    // 从中央缓存获取指定范围的内存块
    void* MahjongCentralCache::GetCacheByRange(size_t nIndex, size_t nBatchNum)
    {
        // 参数有效性检查
        if (nIndex >= FREE_LIST_SIZE || nBatchNum == 0)
        {
            return nullptr;  // 非法索引或请求数量为0
        }

        // 使用自旋锁获取索引对应的锁（内存序：获取语义保证后续操作在锁保护下）
        while (m_Locks[nIndex].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();  // 让出CPU时间片等待锁
        }

        void* pstResult = nullptr;
        try
        {
            // 原子加载当前空闲链表头节点（内存序：宽松模式，不保证同步）
            pstResult = m_CentralFreeList[nIndex].load(std::memory_order_relaxed);

            if (!pstResult)  // 如果当前链表为空
            {
                // 计算当前索引对应的内存块大小
                size_t nSize = (nIndex + 1) * ALIGNMENT;
                // 从页缓存获取新内存块
                pstResult = GetCacheByPageCacheSize(nSize);
                if (!pstResult)
                {
                    m_Locks[nIndex].clear(std::memory_order_release);  // 释放锁
                    return nullptr;  // 页缓存分配失败
                }

                // 将大块内存分割为小内存块并构建链表
                char* pstStart = static_cast<char*>(pstResult);
                size_t nTotalBlocks = (PAGECACHESIZE * MahjongPageCache::PAGESIZE) / nSize;  // 总可用块数
                size_t nAllocBlocks = std::min(nBatchNum, nTotalBlocks);  // 实际分配块数

                if (nAllocBlocks > 1)  // 需要构建链表
                {
                    // 构建分配块的链表（隐式链表，利用内存块头部存储下一节点指针）
                    for (size_t i = 1; i < nAllocBlocks; ++i)
                    {
                        void* pstCurrent = pstStart + (i - 1) * nSize;  // 当前块地址
                        void* pstNext = pstStart + i * nSize;            // 下一块地址
                        *reinterpret_cast<void**>(pstCurrent) = pstNext;  // 写入指针
                    }

                    *reinterpret_cast<void**>(pstStart + (nAllocBlocks - 1) * nSize) = nullptr;  // 链表结尾
                }

                // 处理剩余未分配的内存块
                if (nTotalBlocks > nAllocBlocks)
                {
                    void* pstRemainStart = pstStart + nAllocBlocks * nSize;  // 剩余内存起始地址
                    // 构建剩余内存链表
                    for (size_t i = nAllocBlocks + 1; i < nTotalBlocks; ++i)
                    {
                        void* pstCurrent = pstStart + (i - 1) * nSize;
                        void* pstNext = pstStart + i * nSize;
                        *reinterpret_cast<void**>(pstCurrent) = pstNext;
                    }

                    *reinterpret_cast<void**>(pstStart + (nTotalBlocks - 1) * nSize) = nullptr;
                    // 存储剩余链表到中央空闲列表（内存序：释放语义保证可见性）
                    m_CentralFreeList[nIndex].store(pstRemainStart, std::memory_order_release);
                }
            }
            else  // 当前链表有可用内存
            {
                void* pstCurrent = pstResult;  // 当前遍历指针
                void* pstTemp = nullptr;       // 前驱指针
                size_t nCount = 0;              // 计数

                // 遍历链表获取指定数量的内存块
                while (pstCurrent && nCount < nBatchNum)
                {
                    pstTemp = pstCurrent;
                    pstCurrent = *reinterpret_cast<void**>(pstCurrent);  // 跳转到下一节点
                    nCount++;
                }

                // 断开链表连接
                if (pstTemp)
                {
                    *reinterpret_cast<void**>(pstTemp) = nullptr;  // 截断链表
                }
                // 更新中央空闲链表头（注意：原代码拼写错误应为release）
                m_CentralFreeList[nIndex].store(pstCurrent, std::memory_order_release);
            }
        }
        catch (...)  // 异常处理
        {
            m_Locks[nIndex].clear(std::memory_order_release);  // 释放锁
            throw;  // 重新抛出异常
        }

        m_Locks[nIndex].clear(std::memory_order_release);  // 正常释放锁
        return pstResult;
    }

    // 将内存块归还到中央缓存
    void MahjongCentralCache::SetCacheByRange(void* pstStart, size_t nSize, size_t nIndex)
    {
        // 参数检查
        if (!pstStart || nIndex >= FREE_LIST_SIZE)
        {
            return;  // 空指针或非法索引
        }

        // 获取自旋锁
        while (m_Locks[nIndex].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        try
        {
            void* pstEnd = pstStart;  // 遍历指针
            size_t nCount = 1;        // 计数

            // 查找链表末尾
            while (*reinterpret_cast<void**>(pstEnd) != nullptr && nCount < nSize)
            {
                pstEnd = *reinterpret_cast<void**>(pstEnd);  // 移动到下一节点
                nCount++;
            }

            // 将归还链表插入到中央空闲链表头部
            void* pstCurrent = m_CentralFreeList[nIndex].load(std::memory_order_relaxed);
            *reinterpret_cast<void**>(pstEnd) = pstCurrent;   // 原链表头接到新链表尾
            m_CentralFreeList[nIndex].store(pstStart, std::memory_order_release);  // 更新链表头
        }
        catch (...)
        {
            m_Locks[nIndex].clear(std::memory_order_release);
            throw;
        }

        m_Locks[nIndex].clear(std::memory_order_release);  // 释放锁
    }

    // 从页缓存获取内存块
    void* MahjongCentralCache::GetCacheByPageCacheSize(size_t nSize)
    {
        // 计算需要的页数（向上取整）
        size_t nPageNum = (nSize + MahjongPageCache::PAGESIZE - 1) / MahjongPageCache::PAGESIZE;

        // 根据大小选择分配策略
        if (nSize <= PAGECACHESIZE * MahjongPageCache::PAGESIZE)
        {
            // 分配固定大小的页缓存块（注意：函数名疑似拼写错误应为GetCacheByPageNum）
            return MahjongPageCache::GetInstance().NewCacheByPageNum(PAGECACHESIZE);
        }
        else
        {
            // 按需分配页数
            return MahjongPageCache::GetInstance().NewCacheByPageNum(nPageNum);
        }
    }
}
