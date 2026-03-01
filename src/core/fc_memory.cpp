//>--- fc_memory.cpp ---<//
#include "fc_memory.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "assert.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <tlsf.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstring>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


#if defined (FROLIC_MEMORY_STACK)
// #include "../external/StackWalker.h"
#endif

#define HEAP_ALLOCATOR_STATS

//
//
namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY DEBUG   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // #define FROLIC_MEMORY_DEBUG
  // BUG I think that mem_assert should be switched to something else... I think it's undefined??
#if defined (FROLIC_MEMORY_DEBUG)
	#define hy_mem_assert(cond) hy_assert(cond)
#else
	#define hy_mem_assert(cond)
#endif


  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY SERVICE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  static MemoryService sMemoryService;
  // Locals
  static sizeT sSize = megabytes(32) + tlsf_size() + 8;
  // Walker Methods
  static void exitWalker(void* ptr, sizeT size, int used, void* user);
  // TODO add IMGUI walker method
  //
  MemoryService* MemoryService::instance() { return &sMemoryService; }

  //
  //
  void MemoryService::init(void* configuration)
  {
    fcPrint("Memory Service Initialization\n");

    MemoryServiceConfiguration* memConfig = static_cast<MemoryServiceConfiguration*>(configuration);

    systemAllocator.init(memConfig ? memConfig->maxDynamicSize : sSize);
  }

  //
  //
  void MemoryService::shutdown()
  {
    systemAllocator.shutdown();

    fcPrint("Memory Service Shutdown\n");
  }

  //
  //
  void exitWalker(void* ptr, sizeT size, int used, void* user)
  {
    MemoryStats* stats = (MemoryStats*)user;
    stats->add(used ? size : 0);

    if (used)
    {
      fcPrint("Found active allocation %p, %llu\n", ptr, size);
    }
  }

  // TODO add IMGUI walker and draw functions

  //
  //
  // TODO implement
  void MemoryService::test()
  {
    // static u8 mem[1024];
    // LinearAllocator la;
    // la.init(mem, 1025);
  }


  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY STRUCTURES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  //
  void HeapAllocator::init(sizeT size)
  {
    // allocate
    memory = malloc(size);
    maxSize = size;
    allocatedSize = 0;

    tlsfHandle = tlsf_create_with_pool(memory, size);

    fcPrint("Heap Allocator of size %llu created\n", size);
  }

  //
  //
  void HeapAllocator::shutdown()
  {
    // check memory at the exit of application
    MemoryStats stats{0, maxSize};
    pool_t pool = tlsf_get_pool(tlsfHandle);
    tlsf_walk_pool(pool, exitWalker, (void*)&stats);

    if (stats.allocatedBytes)
    {
      fcPrint("Heap Allocator Shutdown./n==============\nFAILURE! Allocated memory detected...");
      fcPrint("allocated %llu\n============\n\n", stats.allocatedBytes, stats.totalBytes);
    }
    else
    {
      fcPrint("Heap Allocator Shutdown - all memory free!\n");
    }

    FCASSERTM(stats.allocatedBytes == 0, "Allocations still present! This indicates a bug in program!");

    tlsf_destroy(tlsfHandle);

    free(memory);
  }

  //
  // ?? wondering why this is in parenthesis here
#if defined (FROLIC_MEMORY_STACK)
  class FcStackWalker : public StackWalker
  {
   protected:
     virtual void OnOutput(LPCSTR szText)
      {
        fcPrint("\nStack: \n%s\n", szText);
        StackWalker::OnOutpu(szText);
      }
   public:
     FcStackWalker() : StackWalker() {}
  };

  //
  //
  void* HeapAllocator::allocate(sizeT size, sizeT alignment)
  {
    // if (size == 16)
    // {
    //   FcStackWalker sw;
    //   sw.ShowCallstack();
    // }
    void* mem = tlsf_malloc(tlsfHandle, size);
    fcPrint("Mem: %p, size %llu \n", mem, size);
    return mem;
  }
  //
#else
  //
  void* HeapAllocator::allocate(sizeT size, sizeT alignment)
  {
#if defined (HEAP_ALLOCATOR_STATS)
    void* allocatedMemory = alignment == 1 ? tlsf_malloc(tlsfHandle, size)
                            : tlsf_memalign(tlsfHandle, alignment, size);

    sizeT actualSize = tlsf_block_size(allocatedMemory);
    allocatedSize += actualSize;

    return allocatedMemory;
    //
#else // not defined (HEAP_ALLOCATOR_STATS)
    //
    return tlsf_malloc(tlsfHandle, size);
#endif //  (HEAP_ALLOCATOR_STATS)
  }
#endif // defined (FROLIC_MEMORY_STACK)

  //
  //
  void* HeapAllocator::allocate(sizeT size, sizeT alignment, cstring file, i32 line)
  {
    return allocate(size, alignment);
  }

  //
  //
  void HeapAllocator::deallocate(void *ptr)
  {
#if defined (HEAP_ALLOCATOR_STATS)
    sizeT actualSize = tlsf_block_size(ptr);
    allocatedSize -= actualSize;
#endif
    tlsf_free(tlsfHandle, ptr);
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   LINEAR ALLOCATOR   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  //
  void LinearAllocator::init(sizeT size)
  {
    memory = (u8*)malloc(size);
    totalSize = size;
    allocatedSize = 0;
  }

  //
  //
  void* LinearAllocator::allocate(sizeT size, sizeT alignment)
  {
    FCASSERT(size > 0);

    const sizeT newStart = memoryAlign(allocatedSize, alignment);
    const sizeT newAllocatedSize = newStart + size;

    if (newAllocatedSize > totalSize)
    {
      hy_mem_assert(false && "Linear Memory Overflow");
      return nullptr;
    }

    allocatedSize = newAllocatedSize;

    return memory + newStart;
  }

  //
  //
  void* LinearAllocator::allocate(sizeT size, sizeT alignment, cstring file, i32 line)
  {
    return allocate(size, alignment);
  }

  //
  //
  void LinearAllocator::deallocate(void *ptr)
  {
    // This allocator does not allocate on a per-pointer basis!
  }

  //
  //
  void LinearAllocator::clear()
  {
    allocatedSize = 0;
  }

  //
  //
  void LinearAllocator::shutdown()
  {
    clear();
    free(memory);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY METHODS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  // ?? Seems kind of pointless to have this wrapper function
  void memoryCopy(void* destination, void* source, sizeT size)
  {
    memcpy(destination, source, size);
  }

  //
  //
  sizeT memoryAlign(sizeT size, sizeT alignment)
  {
    const sizeT alignmentMask = alignment - 1;
    return (size + alignmentMask) & ~alignmentMask;
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   MALLOC ALLOCATOR   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  //
  void* MallocAllocator::allocate(sizeT size, sizeT alignment)
  {
    return malloc(size);
  }

  //
  //
  void* MallocAllocator::allocate(sizeT size, sizeT alignment, cstring file, i32 line)
  {
    return malloc(size);
  }

  //
  //
  void MallocAllocator::deallocate(void *ptr)
  {
    free(ptr);
  }


  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STACK ALLOCATOR   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  void StackAllocator::init(sizeT size)
  {
    memory = (u8*)malloc(size);
    allocatedSize = 0;
    totalSize = size;
  }

  //
  //
  void* StackAllocator::allocate(sizeT size, sizeT alignment)
  {
    FCASSERT(size > 0);

    const sizeT newStart = memoryAlign(allocatedSize, alignment);
    const sizeT newAllocatedSize = newStart + size;

    if (newAllocatedSize > totalSize)
    {
      hy_mem_assert(false && "Linear Memory Overflow");
      return nullptr;
    }

    allocatedSize = newAllocatedSize;
    return memory + newStart;
  }

  //
  //
  void* StackAllocator::allocate(sizeT size, sizeT alignment, cstring file, i32 line)
  {
    return allocate(size, alignment);
  }

  //
  //
  void StackAllocator::deallocate(void *ptr)
  {
    FCASSERT(ptr >= memory);
    FCASSERTM(ptr < memory + totalSize, "Out of bound free on linear allocator (outside bounds). Atempting to to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", (u8*)ptr, (u8*)ptr - memory, memory, totalSize, allocatedSize);

    FCASSERTM(ptr < memory + allocatedSize, "Out of bounds free on linear allocator (inside bounds, after allocated). Atempting to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", (u8*)ptr, (u8*)ptr - memory, memory, totalSize, allocatedSize);

    const sizeT sizeAtPtr = (u8*)ptr - memory;
    allocatedSize = sizeAtPtr;
  }

  //
  //
  sizeT StackAllocator::getMarker()
  {
    return allocatedSize;
  }

  //
  //
  void StackAllocator::freeMarker(sizeT marker)
  {
    const sizeT difference = marker - allocatedSize;

    if (difference > 0)
    {
      allocatedSize = marker;
    }
  }

  //
  //
  void StackAllocator::clear()
  {
    allocatedSize = 0;
  }

  //
  //
  void StackAllocator::shutdown()
  {
    free(memory);
  }


  // -*-*-*-*-*-*-*-*-*-*-*-*-   DOUBLE STACK ALLOCATOR   -*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  //
  void DoubleStackAllocator::init(sizeT size)
  {
    memory = (u8*)malloc(size);
    top = size;
    // ?? technically, bottom is already 0 but that brings up a point, are we allowed to
    // init an already used double stack
    bottom = 0;
    totalSize = size;
  }

  //
  //
  void* DoubleStackAllocator::allocate(sizeT size, sizeT alignment)
  {
    FCASSERT(false);
    return nullptr;
  }

  //
  //
  void* DoubleStackAllocator::allocate(sizeT size, sizeT alignment, cstring file, i32 line)
  {
    FCASSERT(false);
    return nullptr;
  }

  //
  //
  void* DoubleStackAllocator::allocateTop(sizeT size, sizeT alignment)
  {
    FCASSERT(size > 0);

    const sizeT newStart = memoryAlign(top - size, alignment);
    if (newStart <= bottom)
    {
      hy_mem_assert(false && "Memory Overflow Crossing Top of Double Stack");
      return nullptr;
    }

    top = newStart;
    return memory + newStart;
  }

  //
  //
  void* DoubleStackAllocator::allocateBottom(sizeT size, sizeT alignment)
  {
    FCASSERT(size > 0);

    const sizeT newStart = memoryAlign(bottom, alignment);
    const sizeT newAllocatedSize = newStart + size;

    if (newAllocatedSize >= top)
    {
      hy_mem_assert(false && "Memory Overflow Crossing Bottom of Double Stack");
      return nullptr;
    }

    bottom = newAllocatedSize;
    return memory + newStart;
  }

  //
  //
  sizeT DoubleStackAllocator::getTopMarker()
  {
    return top;
  }

  //
  //
  sizeT DoubleStackAllocator::getBottomMarker()
  {
    return bottom;
  }



  //
  //
  void DoubleStackAllocator::deallocate(void *ptr)
  {
    FCASSERT(false);
  }

  //
  //
  void DoubleStackAllocator::deallocateTop(sizeT size)
  {
    if (size > totalSize - top)
    {
      top = totalSize;
    }
    else
    {
      top += size;
    }
  }

  //
  //
  void DoubleStackAllocator::deallocateBottom(sizeT size)
  {
    if (size > bottom)
    {
      bottom = 0;
    }
    else
    {
      bottom -= size;
    }
  }

  //
  //
  void DoubleStackAllocator::shutdown()
  {
    free(memory);
  }





}// --- namespace fc --- (END)
