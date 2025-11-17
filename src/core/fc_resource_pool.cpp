#include "fc_resource_pool.hpp"

#include "assert.hpp"
#include "log.hpp"
#include <cstring>


namespace fc
{
  static const uint32_t FC_INVALID_INDEX = 0xffffffff;


  void ResourcePool::init(FcAllocator* allocator, uint32_t poolSize, uint32_t resourceSize)
  {
    pAllocator = allocator;
    mPoolSize = poolSize;
    mResourceSize = resourceSize;

    // ?? determine if there is some reason the pool needs to be allocated this way instead
    // of say, an initialized vector or other data structure. One thing that would change
    // is we would no longer need the init fuction to initialize default values within a class
    // Group allocate (resource size + uint32)
    sizeT allocationSize = poolSize * (resourceSize + sizeof(u32));
    mMemory = (u8*)allocator->allocate(allocationSize, 1);
    memset(mMemory, 0, allocationSize);

    // Allocate and add free indices
    mFreeIndices = (u32*)(mMemory + poolSize * resourceSize);
    mFreeIndicesHead = 0;

    for (u32 i = 0; i < poolSize; ++i)
    {
      mFreeIndices[i] = i;
    }

    mUsedIndices = 0;
  }

  //
  //
  void ResourcePool::shutdown()
  {
    if (mFreeIndicesHead != 0)
    {
      fcPrint("Memory Leak: ResourcePool has unfreed resources.\n");

      for (u32 i = 0; i < mFreeIndicesHead; i++)
      {
        fcPrint("\tResource %u\n", mFreeIndices[i]);
      }
    }

    FCASSERT(mUsedIndices == 0);

    pAllocator->deallocate(mMemory);
  }

  //
  //
  void ResourcePool::freeAll()
  {
    mFreeIndicesHead = 0;
    mUsedIndices = 0;

    for (u32 i = 0; i < mPoolSize; ++i)
    {
      mFreeIndices[i] = i;
    }
  }

  //
  //
  u32 ResourcePool::getNextResourceIndex()
  {
    // TODO add bits for checking if resource is alive and use bitmasks
    if (mFreeIndicesHead < mPoolSize)
    {
      const u32 freeIndex = mFreeIndices[mFreeIndicesHead++];
      ++mUsedIndices;

      // ?? drops the const, should use for index
      return freeIndex;
    }
    // ERROR: no more resources left!
    FCASSERT(false);
    return FC_INVALID_INDEX;
  }





  //
  //
  void ResourcePool::release(u32 handle)
  {
    // TODO add bits for checking if resource is alive and use bitmasks

    mFreeIndices[--mFreeIndicesHead] = handle;
    --mUsedIndices;
  }

  //
  //
  void* ResourcePool::get(u32 handle)
  {
    if (handle != FC_INVALID_INDEX)
    {
      return &mMemory[handle * mResourceSize];
    }

    return nullptr;
  }

  //
  //
  const void* ResourcePool::get(u32 handle) const
  {
    if (handle != FC_INVALID_INDEX)
    {
      // const void* resourcePtr = &mMemory[handle * mResourceSize];
      // (FcImage*)resourcePtr->init();
      return &mMemory[handle * mResourceSize];
    }
    return nullptr;
  }

}// --- namespace fc --- (END)
