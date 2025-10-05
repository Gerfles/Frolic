#include "fc_locator.hpp"

#include "core/fc_billboard_render_system.hpp"
#include "core/fc_light.hpp"
#include "core/fc_renderer.hpp"
#include "vulkan/vulkan_core.h"

namespace fc
{
  FcRenderer* FcLocator::pRenderer;
  FcGpu* FcLocator::pGpu;
  VkDevice FcLocator::pDevice;
  VkPhysicalDevice FcLocator::pPhysicalDevice;
   //static Audio* pAudioService;
  FcDescriptorClerk* FcLocator::pDescriptorClerk;
  VkExtent2D FcLocator::mScreenDimensions;


   // initialze game object list to maximum size allowed
//  std::vector<FcGameObject*> FcLocator::mGameObjectsList(MAX_GAME_OBJECTS, nullptr);
  /* std::vector<FcGameObject*> FcLocator::mGameObjectsList;//(100, nullptr); */
   //
  std::vector<FcLight*> FcLocator::mLightsList;
   //
  std::vector<FcBillboard* > FcLocator::mBillboardsList;


  void FcLocator::init()
  {
    pGpu = nullptr;
    pDevice = nullptr;
    pPhysicalDevice = nullptr;
    pDescriptorClerk = nullptr;
  }

  void FcLocator::provide(FcRenderer* renderer)
  {
    if (renderer == nullptr)
    {
       // revert to default null device
      std::cout << "Failed to assign proper FcGpu pointer to locator" << std::endl;
    }
    else
    {
      pRenderer = renderer;
    }
  }

  FcRenderer& FcLocator::Renderer()
  {
    if (pRenderer == nullptr)
    {
       // revert to default null device
      std::cout << "Requested Renderer but none was provided!" << std::endl;
    }
    return *pRenderer;
  }



  void FcLocator::provide(FcGpu* gpu)
        {
          if (gpu == nullptr)
          {
             // revert to default null device
            std::cout << "Failed to assign proper FcGpu pointer to locator" << std::endl;
//              pGpu = &mNullGpu;
          }
          else
          {
             // TODO should check for null values here as well
            pGpu = gpu;
            pDevice = gpu->getVkDevice();
             //pPhysicalDevice = nullptr;
            pPhysicalDevice = gpu->physicalDevice();
          }
        }

  void FcLocator::provide(FcDescriptorClerk* descriptorClerk)
        {
          if (descriptorClerk == nullptr)
          {
             // revert to default null device
            std::cout << "Failed to assign proper FcDescriptor pointer to locator" << std::endl;
//            pDescriptorClerk = &mNullDescriptorClerk;
          }
          else
          {
            pDescriptorClerk = descriptorClerk;
          }
        }


  void FcLocator::provide(VkExtent2D screenDimensions)
  {
    if (screenDimensions.width <= 0 || screenDimensions.height <= 0)
    {
      std::cout << "Failed to assign proper screen dimensions to locator!" << std::endl;
    }
    else
    {
      mScreenDimensions = screenDimensions;
    }

  }

} // _END_ namespace fc
