#include "fc_resource_pool.hpp"


namespace fc
{
  static const uint32_t FC_INVALID_INDEX = 0xffffffff;


  void ResourcePool::init(uint32_t poolSize, uint32_t resourceSize)
  {
    mPoolSize = poolSize;
    mResourceSize = resourceSize;

    // Group allocate (resource size + uint32)
    size_t allocationSize = poolSize * (resourceSize + sizeof(uint32_t));
    mMemory = (uint8_t*)
  }

}
