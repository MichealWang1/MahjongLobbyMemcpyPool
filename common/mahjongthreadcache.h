/*
   @Time     : 2018/7/10 10:18
   @Author   : 王一冰
   @Describe :
   @Software : Copyright (c) 2018 中国手游娱乐集团有限公司
*/
#pragma  once
#include "common.h"

namespace MahjongMemoryPool 
{
    // 线程本地缓存
    class MahjongThreadCache
    {
    public:
        static MahjongThreadCache* GetInstance()
        {
            static thread_local MahjongThreadCache stThreadCacheInstance;
            return &stThreadCacheInstance;
        }

        void* MahjongNewCache(size_t nSize);
        void  MahjongDeleteCache(void* pstCache, size_t nSize);
    private:
        MahjongThreadCache() = default;
        // 获取内存从中心缓存
        void* GetCacheByCentralCache(size_t nIndex);
        // 设置内存到中心缓存
        void SetCacheToCentralCache(void* pstCacheStart, size_t nSize);
        // 计算批量获取内存块的数量
        size_t GetBatchNumByCentralCache(size_t nSize);
        // 判断是否需要归还内存给中心缓存
        bool CheckIsReturnCacheToByCacheCentral(size_t nSize);
    private:
        // 每个线程的自由链表数组
        std::array<void*, FREE_LIST_SIZE> m_FreeList;
        // 自由链表大小统计
        std::array<size_t, FREE_LIST_SIZE> m_FreeListSize;
    };
}
