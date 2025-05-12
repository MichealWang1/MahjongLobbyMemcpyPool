/*
   @Time     : 2018/7/10 16:32
   @Author   : 王一冰
   @Describe :
   @Software : Copyright (c) 2018 中国手游娱乐集团有限公司
*/
#pragma once
#include "common.h"
#include <mutex>

namespace MahjongMemoryPool 
{
	class MahjongCentralCache
	{
	public:
		static MahjongCentralCache& GetInstance() 
        {
			static MahjongCentralCache stCentralCacheInstance;
			return stCentralCacheInstance;
		}

		void* GetCacheByRange(size_t nIndex, size_t nBatchNum);
		void SetCacheByRange(void* pstStart, size_t nSize, size_t nIndex);
	private:
        // 相互是还所有原子指针为nullptr
		MahjongCentralCache() 
		{
			// 初始化中心缓存的自由链表
			for (auto& ptr : m_CentralFreeList)
			{
				ptr.store(nullptr, std::memory_order_relaxed);
			}

			// 初始化同步的自旋锁
			for (auto& lock : m_Locks)
			{
				lock.clear();
			}
		}

        // 从页缓存获取内存
		void* GetCacheByPageCacheSize(size_t nSize);

    private:
        // 中心缓存的自由链表
		std::array<std::atomic<void*>, FREE_LIST_SIZE> m_CentralFreeList;

        // 用于同步的自旋锁
		std::array<std::atomic_flag, FREE_LIST_SIZE> m_Locks;
	};
}