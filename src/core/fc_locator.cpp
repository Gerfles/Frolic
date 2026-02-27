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
  // TODO make this part of drawCollection and delete
  /* std::vector<FcBillboard* > FcLocator::mBillboardsList; */


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
      fcPrintEndl("Failed to assign proper FcGpu pointer to locator");
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
      fcPrintEndl("Requested Renderer but none was provided!");
    }
    return *pRenderer;
  }



  void FcLocator::provide(FcGpu* gpu)
        {
          if (gpu == nullptr)
          {
             // revert to default null device
            fcPrintEndl("Failed to assign proper FcGpu pointer to locator");
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
