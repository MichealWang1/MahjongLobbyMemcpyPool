#include "mahjongthreadcache.h"
#include "majhongcentralcache.h"
// 麻将线程缓存类 - 用于管理线程本地内存块的分配和释放
// (采用类似TCMalloc线程缓存机制的设计)
namespace MahjongMemoryPool 
{
  // 分配指定大小的内存块
    void* MahjongThreadCache::MahjongNewCache(size_t nSize)
    {
        // 处理0字节请求，使用最小对齐大小
        if (0 == nSize)
        {
            nSize = ALIGNMENT;  // ALIGNMENT应为预定义的内存对齐值（如8/16字节）
        }

        // 大对象直接走系统分配
        if (nSize > MAX_BYTES)  // MAX_BYTES应为小对象阈值（如256KB）
        {
            // 注意：这里参数应为nSize，原代码中的size可能是笔误
            return malloc(nSize);
        }

        // 通过大小分类器获取对应的自由链表索引
        size_t nIndex = MahJongSizeClass::GetIndex(nSize);
        m_FreeListSize[nIndex]--;  // 减少对应链表的可用计数

        // 尝试从自由链表获取缓存块
        void* pstTemp = m_FreeList[nIndex];
        if (pstTemp)  // 链表中有可用块
        {
            // 使用链表头部的块，并更新链表头为下一个节点
            m_FreeList[nIndex] = *reinterpret_cast<void**>(pstTemp);
            return pstTemp;
        }

        // 链表为空时，从中心缓存批量获取
        return GetCacheByCentralCache(nIndex);
    }

    // 释放内存块到线程缓存
    void MahjongThreadCache::MahjongDeleteCache(void* pstCache, size_t nSize)
    {
        // 大对象直接释放给系统
        if (nSize > MAX_BYTES)
        {
            free(pstCache);
            return;
        }

        // 获取对应的自由链表索引
        size_t nIndex = MahJongSizeClass::GetIndex(nSize);

        // 将释放的块插入链表头部
        *reinterpret_cast<void**>(pstCache) = m_FreeList[nIndex];
        m_FreeList[nIndex] = pstCache;
        m_FreeListSize[nIndex]++;  // 增加可用计数

        // 检查是否需要归还部分缓存到中心缓存
        if (CheckIsReturnCacheToByCacheCentral(nIndex))
        {
            SetCacheToCentralCache(m_FreeList[nIndex], nSize);
        }
    }

    // 从中心缓存获取批量内存块
    void* MahjongThreadCache::GetCacheByCentralCache(size_t nIndex)
    {
        // 计算实际分配大小（基于索引的对齐大小）
        size_t nSize = (nIndex + 1) * ALIGNMENT;

        // 获取建议的批量数量（根据内存大小决定）
        size_t nBatchNum = GetBatchNumByCentralCache(nSize);

        // 从中心缓存获取内存块范围
        void* pstStart = MahjongCentralCache::GetInstance().GetCacheByRange(nIndex, nBatchNum);
        if (pstStart == nullptr)
        {
            return nullptr;
        }

        // 更新本地缓存链表（可能需要修改）
        // 注意：原代码此处m_FreeList[nIndex] += nBatchNum; 可能存在问题，应为链表操作
        // 正确实现可能需要将批量获取的块链接到自由链表

        void* pstResult = pstStart;  // 返回第一个可用块

        if (nBatchNum > 1)
        {
            // 将剩余块链接到自由链表
            m_FreeList[nIndex] = *reinterpret_cast<void**>(pstStart);
        }

        return pstResult;
    }

    // 归还多余缓存到中心缓存
    void MahjongThreadCache::SetCacheToCentralCache(void* pstCacheStart, size_t nSize)
    {
        size_t nIndex = MahJongSizeClass::GetIndex(nSize);
        size_t nAlignedSize = MahJongSizeClass::RoundUp(nSize);  // 获取对齐后的大小

        size_t nBatchNum = m_FreeListSize[nIndex];
        if (nBatchNum <= 1)  // 数量太少无需归还
        {
            return;
        }

        // 计算需要保留的数量（保留25%或至少1个）
        size_t nKeepNum = std::max(nBatchNum / 4, size_t(1));
        size_t nResultNum = nBatchNum - nKeepNum;  // 实际归还数量

        // 遍历链表找到分割点
        char* pstCurrent = static_cast<char*>(pstCacheStart);
        char* pstNode = pstCurrent;
        for (size_t i = 0; i < nKeepNum; ++i)
        {
            pstNode = reinterpret_cast<char*>(reinterpret_cast<void**>(pstNode));
            if (pstNode == nullptr)  // 链表长度不足时调整
            {
                nResultNum = nBatchNum - (i + 1);
                break;
            }
        }

        if (pstNode != nullptr)
        {
            // 分割链表
            void* pstNextNode = *reinterpret_cast<void**>(pstNode);
            *reinterpret_cast<void**>(pstNode) = nullptr;  // 切断链表

            // 更新本地缓存信息
            m_FreeList[nIndex] = pstCacheStart;
            m_FreeListSize[nIndex] = nKeepNum;

            // 归还剩余块到中心缓存
            if (nResultNum > 0 && pstNextNode != nullptr)
            {
                MahjongCentralCache::GetInstance().SetCacheByRange(
                    pstNextNode,
                    nResultNum * nAlignedSize,
                    nIndex
                );
            }
        }
    }

    // 根据对象大小确定批量获取数量
    size_t MahjongThreadCache::GetBatchNumByCentralCache(size_t nSize)
    {
        size_t nBaseNum = 0;  // 基础批量数

        // 根据大小分级设置基础批量数
        if (nSize <= 32)
        {
            nBaseNum = 64;
        }
        else if (nSize <= 64)
        {
            nBaseNum = 32;
        }
        else if (nSize <= 128)
        {
            nBaseNum = 16;
        }
        else if (nSize <= 256)
        {
            nBaseNum = 8;
        }
        else if (nSize <= 512)
        {
            nBaseNum = 4;
        }
        else if (nSize <= 1024)
        {
            nBaseNum = 2;
        }

        // 计算最大批量数（不超过总缓存大小）
        size_t nMaxNum = std::max(size_t(1), MAXBATCHSIZE / nSize);
        return std::max(size_t(1), std::min(nMaxNum, nBaseNum));
    }

    // 检查是否需要归还缓存到中心缓存
    bool MahjongThreadCache::CheckIsReturnCacheToByCacheCentral(size_t nIndex)
    {
        // 当自由链表中的块数超过系统阈值时触发归还
        return (m_FreeListSize[nIndex] > SystemThreshold);  // SystemThreshold应为预定义阈值
    }
}
