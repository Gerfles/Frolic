//>--- fc_locator.cpp ---<//
#include "fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
#include "fc_gpu.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  FcRenderer* FcLocator::pRenderer;
  FcGpu* FcLocator::pGpu;
  FcDescriptorClerk* FcLocator::pDescriptorClerk;
  FcJanitor* FcLocator::pJanitor;
  //
  VkDevice FcLocator::pDevice;
  VkPhysicalDevice FcLocator::pPhysicalDevice;
  //static Audio* pAudioService;
  // DELETE
  VkExtent2D FcLocator::mScreenDimensions;
  VkInstance FcLocator::mInstance;


  // initialze game object list to maximum size allowed
//  std::vector<FcGameObject*> FcLocator::mGameObjectsList(MAX_GAME_OBJECTS, nullptr);
  /* std::vector<FcGameObject*> FcLocator::mGameObjectsList;//(100, nullptr); */
  //
  std::vector<FcLight*> FcLocator::mLightsList;
  //
  // TODO make this part of drawCollection and delete
  /* std::vector<FcBillboard* > FcLocator::mBillboardsList; */


  void FcLocator::init()
  {
    pGpu = nullptr;
    pDevice = nullptr;
    pPhysicalDevice = nullptr;
    pDescriptorClerk = nullptr;
    pJanitor = nullptr;
  }


  //
  void FcLocator::provide(FcRenderer* renderer)
  {
    pRenderer = renderer;

    if (renderer == nullptr)
    {
      fcPrintEndl("Failed to assign proper FcGpu pointer to locator");
    }
  }


  //
  void FcLocator::provide(FcJanitor* janitor)
  {
    pJanitor = janitor;

    if (pJanitor == nullptr)
    {
      fcPrintEndl("Failed to assign proper FcJanitor pointer to locator");
    }
  }


  //
  FcRenderer& FcLocator::Renderer()
  {
    if (pRenderer == nullptr)
    {
      // revert to default null device
      fcPrintEndl("Requested Renderer but none was provided!");
    }
    return *pRenderer;
  }


  //
  void FcLocator::provide(FcGpu* gpu)
  {
    if (gpu == nullptr)
    {
      fcPrintEndl("Failed to assign proper FcGpu pointer to locator");
      return;
    }

    // TODO should check for null values here as well
    pGpu = gpu;
    pDevice = gpu->getVkDevice();
    //pPhysicalDevice = nullptr;
    pPhysicalDevice = gpu->physicalDevice();
  }


  //
  void FcLocator::provide(VkInstance instance)
  {
    if (instance == nullptr)
    {
      fcPrintEndl("ERROR: Failed to assign proper VkInstance to FcLocator class!");
      return;
    }

    mInstance = instance;
  }


  //
  void FcLocator::provide(FcDescriptorClerk* descriptorClerk)
  {
    if (descriptorClerk == nullptr)
    {
      // revert to default null device
      fcPrintEndl("Failed to assign proper FcDescriptor pointer to locator");
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
      fcPrintEndl("Failed to assign proper screen dimensions to locator!");
    }
    else
    {
      mScreenDimensions = screenDimensions;
    }

  }

} // _END_ namespace fc
