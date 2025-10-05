#pragma once

#include "fc_memory.hpp"
#include <cstdint>
#include <iostream>

namespace fc
{
  //
  struct ResourcePool
  {
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
     void init(FcAllocator* allocator, uint32_t poolSize, uint32_t resourceSize);
     // Returns an index to the resource
     uint32_t getIndex();
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
     T* getIndex();
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
  inline T* ResourcePoolTyped<T>::getIndex()
  {
    uint32_t resourceIndex = ResourcePool::getIndex();

    if (resourceIndex != UINT32_MAX)
    {
      T* resource = get(resourceIndex);
      resource->poolIndex = resourceIndex;
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
    if (mFreeIndicesHead != 0)
    {
      std::cout << "Resource pool has unfreed resources!\n";

      for ( uint32_t i = 0; i < mFreeIndicesHead; ++i)
      {
        // TODO implement fcPrint
        // print all freeIndices[i];
      }
    }
    ResourcePool::shutdown();
  }

}// --- namespace fc --- (END)
