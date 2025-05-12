#include "MahjongLobbyMemcpyPool.h"
#include "common/majhongmemorypool.h"

using namespace std;
using namespace MahjongMemoryPool;

//单元测试相关
void UnitTestBasicAllocation() 
{
	cout << " 开始执行单元测试   BasicAllocationBasicAllocation start" << endl;
    // 测试小内存分配
    void* ptr1 = MahjongMemoryPool::NewMemoryCache(8);
    assert(ptr1 != nullptr);
    MemoryPool::DeleteMemoryCache(ptr1, 8);

    // 测试中等大小内存分配
    void* ptr2 = MemoryPool::NewMemoryCache(1024);
    assert(ptr2 != nullptr);
    MemoryPool::DeleteMemoryCache(ptr2, 1024);

    // 测试大内存分配（超过MAX_BYTES）
    void* ptr3 = MemoryPool::NewMemoryCache(1024 * 1024);
    assert(ptr3 != nullptr);
    MemoryPool::DeleteMemoryCache(ptr3, 1024 * 1024);
    std::cout << "结束执行单元测试 BasicAllocationBasicAllocation end" << std::endl;
}

void UnitTestMomoryWrite() 
{
    cout << " 开始执行单元测试   UnitTestMomoryWrite start" << endl;
    // 分配并写入数据
    const size_t nSize = 128;
    char* pstTemp = static_cast<char*>(MemoryPool::NewMemoryCache(nSize));
    assert(pstTemp != nullptr);

    // 写入数据
    for (size_t i = 0; i < nSize; ++i)
    {
        pstTemp[i] = static_cast<char>(i % 256);
    }

    // 验证数据
    for (size_t i = 0; i < nSize; ++i)
    {
        assert(pstTemp[i] == static_cast<char>(i % 256));
    }

    MemoryPool::deallocate(ptr, size);

    cout << " 开始执行单元测试   UnitTestMomoryWrite end" << endl;
}

// 多线程测试
void UnitTestMultiThreading()
{
    std::cout << "开始执行单元测试 UnitTestMultiThreading start" << std::endl;
    const int ThreadsNum = 4;
    const int ThreadAllocsPre = 1000;
    std::atomic<bool> HasError{ false };



    std::cout << "开始执行单元测试 UnitTestMultiThreading end" << std::endl;
}

// 性能测试相关



int main()
{
	cout << "Hello CMake." << endl;
	return 0;
}
