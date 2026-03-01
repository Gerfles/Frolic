//>--- fc_memory.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
#include "service.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define FC_IMGUI
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MACRO HELPERS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define fcAlloca(size, allocator) ((allocator)->allocate(size, 1, __FILE__, __LINE__))

#define fcAllocam(size, allocator) ((u8*)(allocator)->allocate(size, 1, __FILE__, __LINE__))

#define fcAllocatat(type, allocator) ((type*)(allocator)->allocate(sizeof(type), 1, __FILE__, __LINE__)))

#define fcAllocaa(size, allocator, alignment) ((allocator)->allocate(size, alignment, __FILE__, __LINE__))

#define fcFree(ptr, allocator) (allocator)->deallocate(ptr)

#define kilobytes(size) (size * 1024)
#define megabytes(size) (size * 1024 * 1024)
#define gigabytes(size) (size * 1024 * 1024 * 1024)

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY METHODS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  void memoryCopy(void* destination, void* source, sizeT size);
  // calculate memory size
  sizeT memoryAlign(sizeT size, sizeT alignment);

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY STRUCTURES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  struct MemoryStats
  {
     sizeT allocatedBytes;
     sizeT totalBytes;
     u32 allocationCount;
     //
     void add(sizeT a)
      {
        if (a)
        {
          allocatedBytes += a;
          ++allocationCount;
        }
      }
  };

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY ALLOCATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
    struct FcAllocator
    {
       virtual ~FcAllocator() {}
       virtual void* allocate(sizeT size, sizeT alignment) = 0;
       virtual void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) = 0;
       //
       virtual void deallocate(void* ptr) = 0;
    };

  //
  //
  struct HeapAllocator : public FcAllocator
  {
     void* tlsfHandle;
     void* memory;
     sizeT allocatedSize = 0;
     sizeT maxSize = 0;

     ~HeapAllocator() override {};
     //
     void init(sizeT size);
     //
     void* allocate(sizeT size, sizeT alignment) override;
     void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) override;
     //
     void deallocate(void* ptr) override;
     void shutdown();
  }; // ---   struct FcHeapAllocator : public FcAllocator --- (END)


  //
  //
  struct StackAllocator : public FcAllocator
  {
     u8* memory = nullptr;
     sizeT totalSize = 0;
     sizeT allocatedSize = 0;
     //
     void init(sizeT size);
     //
     void* allocate(sizeT size, sizeT alignment) override;
     void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) override;
     //
     sizeT getMarker();
     // Cleanup
     void deallocate(void* ptr) override;
     void freeMarker(sizeT marker);
     void clear();
     void shutdown();
  };

  //
  //
  struct DoubleStackAllocator : public FcAllocator
  {
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     u8* memory = nullptr;
     sizeT totalSize = 0;
     sizeT top = 0;
     sizeT bottom = 0;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void init(sizeT size);
     //
     void* allocate(sizeT size, sizeT alignment) override;
     void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) override;
     void* allocateTop(sizeT size, sizeT alignment);
     void* allocateBottom(sizeT size, sizeT alignment);
     //
     sizeT getTopMarker();
     sizeT getBottomMarker();
     //
     void freeTopMarker(sizeT marker);
     void freeBottomMarker(sizeT marker);
     //
     void deallocate(void* ptr) override;
     void deallocateTop(sizeT size);
     void deallocateBottom(sizeT size);
     //
     void clearTop();
     void clearBottom();
     void shutdown();
  };

  //
  // Allocator that can only be reset
    struct LinearAllocator : public FcAllocator
    {
       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       u8* memory = nullptr;
       sizeT totalSize = 0;
       sizeT allocatedSize = 0;
       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       void init(sizeT size);
       //
       void* allocate(sizeT size, sizeT alignment) override;
       void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) override;
       // Cleanup
       void deallocate(void* ptr) override;
       void clear();
       void shutdown();
       ~LinearAllocator() {}
    };

  //
  // Danger: this should be used for NON runtime processes, like compilation of resources
  struct MallocAllocator : public FcAllocator
  {
     void* allocate(sizeT size, sizeT alignment) override;
     void* allocate(sizeT size, sizeT alignment, cstring file, i32 line) override;
     void deallocate(void* ptr) override;
  };


  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MEMORY SERVICE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //
  //
    struct MemoryServiceConfiguration
    {
       sizeT maxDynamicSize = 32 * 1024 * 1024; // default 32MB max
    };

  struct MemoryService : public Service
  {
     // ?? look into this singleton pattern
     FC_DECLARE_SERVICE(MemoryService);
     //
     void init(void* configuration);
     void shutdown();
     //
     // TODO include IMGUI functionality
     //
     // Frame allocator
     LinearAllocator scratchAllocator;
     HeapAllocator systemAllocator;
     // Test allocators
     void test();
     //
     static constexpr cstring name = "frolic_memory_service";
  };

}// --- namespace fc --- (END)
