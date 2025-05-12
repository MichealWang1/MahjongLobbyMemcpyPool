#pragma once
#include <sys/mman.h>
#include <cstring>
#include "majhongpagecache.h"

namespace MahjongMemoryPool 
{
	class MemoryPool
	{
	public:
		static void* NewMemoryCache(size_t nSize) 
		{
			return MahjongThreadCache::GetInstance().MahjongNewCache(nSize);
		}

		static void DeleteMemoryCache(void* ptr, size_t nSize) 
		{
			MahjongThreadCache::GetInstance().MahjongDeleteCache(ptr, nSize);
		}
	};
}