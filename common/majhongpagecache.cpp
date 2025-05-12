#include <cstring>
#include "sys/mman.h"
#include "majhongpagecache.h"

namespace MahjongMemoryPool    
{
    // 从系统中分配指定页数的内存
    void* MahjongPageCache::NewCacheByPageNum(size_t nPageNum)
    {
        std::lock_guard<std::mutex> lock(m_MutexLock);  // 加锁保证线程安全

        // 在空闲页链表中查找第一个不小于需求页数的节点
        auto it = m_FreePageNode.lower_bound(nPageNum);
        if (it != m_FreePageNode.end())  // 如果找到合适节点
        {
            PageNode* pstPageNode = it->second;  // 获取空闲页节点

            // 处理链表连接：如果当前节点有后续节点
            if (pstPageNode->pNext)
            {
                m_FreePageNode[it->first] = pstPageNode->pNext;  // 更新哈希表指向下一个节点
            }
            else  // 如果这是最后一个节点
            {
                m_FreePageNode.erase(it);  // 直接从哈希表中删除该条目
            }

            // 如果当前节点页数大于需求，需要分割
            if (pstPageNode->nPageNum > nPageNum)
            {
                PageNode* pstNewPageNode = new PageNode;  // 创建新节点存放剩余页
                // 计算剩余内存块的起始地址
                pstNewPageNode->pPageAddr = static_cast<char*>(pstPageNode->pPageAddr) + nPageNum * PAGESIZE;
                pstNewPageNode->nPageNum = pstPageNode->nPageNum - nPageNum;  // 计算剩余页数
                pstNewPageNode->pNext = nullptr;  // 新节点暂时没有后续节点

                // 将新节点插入对应页数的空闲链表
                auto& pstList = m_FreePageNode[pstNewPageNode->nPageNum];
                pstNewPageNode->pNext = pstList;  // 新节点指向原链表头
                pstList = pstNewPageNode;         // 更新链表头为新节点

                // 调整原节点为实际需求大小
                pstPageNode->nPageNum = nPageNum;
            }

            // 将分配的节点移入工作链表
            m_WorkPageNode[pstPageNode->pPageAddr] = pstPageNode;
            return pstPageNode->pPageAddr;  // 返回分配的内存地址
        }

        // 没有找到合适空闲块，直接向系统申请新内存
        void* pstNewCache = NewCacheBySystem(nPageNum);
        if (pstNewCache == nullptr)
        {
            return nullptr;  // 注意：原代码缺少分号，可能需要修正
        }

        // 创建新页节点记录分配信息
        PageNode* pstNewPageNode = new PageNode;
        pstNewPageNode->pPageAddr = pstNewCache;
        pstNewPageNode->nPageNum = nPageNum;
        pstNewPageNode->pNext = nullptr;

        // 注册到工作链表
        m_WorkPageNode[pstNewPageNode->pPageAddr] = pstNewPageNode;
        return pstNewCache;
    }

    // 释放指定页数的内存
    void MahjongPageCache::DeleteCacheByPageNum(void* ptr, size_t nPageNum)
    {
        std::lock_guard<std::mutex> lock(m_MutexLock);  // 加锁保证线程安全

        // 在工作链表中查找目标节点
        auto pstCurNode = m_WorkPageNode.find(ptr);
        if (pstCurNode == m_WorkPageNode.end())
        {
            return;  // 未找到直接返回（可能已释放）
        }

        PageNode* pstPageNode = pstCurNode->second;  // 获取页节点

        // 计算相邻内存块地址
        void* pstNextAddr = static_cast<char*>(ptr) + nPageNum * PAGESIZE;
        auto pstNextNode = m_WorkPageNode.find(pstNextAddr);

        // 尝试合并后续相邻空闲块
        if (pstNextNode != m_WorkPageNode.end())
        {
            PageNode* pstNextPageNode = pstNextNode->second;
            bool bFind = false;
            auto& pstNextList = m_FreePageNode[pstNextPageNode->nPageNum];

            // 从空闲链表中移除后续节点
            if (pstNextList == pstNextPageNode)  // 如果是链表头
            {
                pstNextList = pstNextPageNode->pNext;
                bFind = true;
            }
            else if (pstNextList)  // 遍历链表查找
            {
                PageNode* pstTemp = pstNextList;
                while (pstTemp->pNext)
                {
                    if (pstTemp->pNext == pstNextPageNode)
                    {
                        pstTemp->pNext = pstNextPageNode->pNext;
                        bFind = true;
                        break;
                    }
                    pstTemp = pstTemp->pNext;
                }
            }

            // 如果找到并移除了后续节点
            if (bFind)
            {
                pstPageNode->nPageNum += pstNextPageNode->nPageNum;  // 合并页数
                m_WorkPageNode.erase(pstNextAddr);  // 从工作链表移除
                delete pstNextPageNode;             // 删除节点对象（注意：原代码写delete pstNextAddr可能有误）
            }
        }

        // 将当前节点插入空闲链表
        auto& pstList = m_FreePageNode[pstPageNode->nPageNum];
        pstPageNode->pNext = pstList;  // 当前节点指向原链表头
        pstList = pstPageNode;         // 更新链表头为当前节点
    }

    // 通过系统调用分配内存页
    void* MahjongPageCache::NewCacheBySystem(size_t nPageNum)
    {
        size_t nSize = nPageNum * PAGESIZE;  // 计算总字节数
        // 使用mmap申请内存（注意参数）:
        // - 匿名私有映射，无文件关联
        // - 可读写权限
        void* pstNewCache = mmap(nullptr, nSize, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (pstNewCache == MAP_FAILED)
        {
            return nullptr;  // 系统分配失败
        }
        memset(pstNewCache, 0, nSize);  // 清空内存
        return pstNewCache;
    }
}