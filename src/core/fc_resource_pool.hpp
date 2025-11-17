#pragma once

#include "core/log.hpp"
#include "fc_memory.hpp"
#include <cstdint>
#include <iostream>

namespace fc
{
  // TODO improve this class with C++ and modern memory management...
  // think about integrating these features into a buffer object if plausible
  // start here: https://gameprogrammingpatterns.com/object-pool.html
  class ResourcePool
  {
   private:
     FcAllocator* pAllocator = nullptr;
     //
     uint8_t* mMemory = nullptr;
     uint32_t* mFreeIndices = nullptr;
     //
     uint32_t mFreeIndicesHead = 0;
     uint32_t mPoolSize = 16;
     uint32_t mResourceSize = 4;
     uint32_t mUsedIndices = 0;
     //
   public:
     void init(FcAllocator* allocator, uint32_t poolSize, uint32_t resourceSize);
     // Returns an index to the resource
     uint32_t getFirstFreeIndex() { return mFreeIndicesHead; }
     uint32_t peekIndexAtPosition(uint32_t index) { return mFreeIndices[index]; }
     uint32_t getNextResourceIndex();
     void* get(uint32_t index);
     const void* get(uint32_t index) const;
     // cleanup
     void release(uint32_t index);
     void freeAll();
     void shutdown();
  }; // ---   struct ResourcePool --- (END)


  //
  template <typename T>
  struct ResourcePoolTyped : public ResourcePool
  {
     void init(FcAllocator* allocator, uint32_t poolSize);
     T* getNextFree();
     T* get(uint32_t index);
     const T* get(uint32_t index) const;
     // cleanup
     void release(T* resource);
     void shutdown();
  };


  // -*-*-*-*-*-*-*-*-*-*-*-*-   TEMPLATE INSTANTIATIONS   -*-*-*-*-*-*-*-*-*-*-*-*- //

  // <T> instantiation of in resource pool init
  template<typename T>
  inline void ResourcePoolTyped<T>::init(FcAllocator* allocator, uint32_t poolSize)
  {
    ResourcePool::init(allocator, poolSize, sizeof(T) );
  }


  //
  template <typename T>
  inline T* ResourcePoolTyped<T>::getNextFree()
  {
    uint32_t resourceIndex = ResourcePool::getNextResourceIndex();

    if (resourceIndex != U32_MAX)
    {
      T* resource = get(resourceIndex);
      // TODO reenable this once resource system gets fully fleshed out
      /* resource->poolIndex = resourceIndex; */
      // TODO get rid of init here eventually (may require use of initializing class alternatively)
      resource->init(resourceIndex);
      return resource;
    }
    return nullptr;
  }


  //
  template <typename T>
  inline T* ResourcePoolTyped<T>::get(uint32_t index)
  {
    return (T*)ResourcePool::get(index);
  }


  //
  template <typename T>
  inline const T* ResourcePoolTyped<T>::get(uint32_t index) const
  {
    return (const T*)ResourcePool::get(index);
  }


  //
  template <typename T>
  inline void ResourcePoolTyped<T>::release(T *resource)
  {
    ResourcePool::release(resource->poolIndex);
  }


//
  template<typename T>
  inline void ResourcePoolTyped<T>::shutdown()
  {
    uint32_t firstFreeIndex = getFirstFreeIndex();

    if (firstFreeIndex != 0)
    {
      std::cout << "Resource pool has unfreed resources!\n";

      for ( uint32_t i = 0; i < firstFreeIndex; ++i)
      {
        uint32_t occupiedIndex = peekIndexAtPosition(i);
        fcPrint("\tResource %u\n", occupiedIndex);
        // print all freeIndices[i];
      }
    }
    ResourcePool::shutdown();
  }

}// --- namespace fc --- (END)
