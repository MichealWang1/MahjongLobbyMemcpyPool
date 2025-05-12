/*
   @Time     : 2018/7/9 11:29
   @Author   : 王一冰
   @Describe :
   @Software : Copyright (c) 2018 中国手游娱乐集团有限公司
*/

#pragma once
#include <map>
#include <mutex>
#include "common.h"
namespace MahjongMemoryPool 
{
	class MahjongPageCache
	{
    public:
        static const size_t PAGESIZE = 4096; // 4K页大小
		static MahjongPageCache& GetInstance() 
		{
			static MahjongPageCache stPageCacheInstance;
			return stPageCacheInstance;
		}
        // 从系统中分配指定页数的内存
		void* NewCacheByPageNum(size_t nPageNum);
        // 释放指定页数的内存
		void DeleteCacheByPageNum(void* ptr, size_t nPageNum);
	private:
		MahjongPageCache() = default;

        // 通过系统调用分配内存页
		void* NewCacheBySystem(size_t nPageNum);
	
	private:
		struct PageNode
		{
			void* pPageAddr;
			size_t nPageNum;
			PageNode* pNext;
		};

        // 按页数管理空闲span，不同页数对应不同PageNode链表
		std::map<size_t, PageNode*> m_FreePageNode;
        
		// 页号到PageNode的映射，工作中的node节点   用于回收
		std::map<void*, PageNode*> m_WorkPageNode;
		std::mutex m_MutexLock;
	};

}