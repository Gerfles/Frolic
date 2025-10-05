#pragma once

#include "vulkan/vulkan_core.h"
#include <vector>


namespace fc
{
  // forward declarations
  class FcDescriptorClerk;
  class FcGpu;
  class FcBillboard;
  /* class FcGameObject; */
  class FcRenderer;
  class FcLight;

//    // TODO think about removing NullServices for release version
//   class NullGpu: public FcGpu
//   {
//    public:
// //     virtual void VkDevice() { std::cout << "No device has been registered"; }
//   };


//   class NullDescriptor: public FcDescriptor
//   {
//    public:
//   };

  class FcLocator
  {
   private:
      // RENDERER SERVICES
     static FcRenderer* pRenderer;
     static FcGpu* pGpu;
     static FcDescriptorClerk* pDescriptorClerk;
     static VkDevice pDevice;
     static VkPhysicalDevice pPhysicalDevice;
     static VkExtent2D mScreenDimensions;
      // this is the global handle table -- a simple array of pointers to FcGameObjects

      // TODO resize to make mgameobjectslist size of MAX_GAME_OBJECTS
     /* static std::vector<FcGameObject* > mGameObjectsList; */
     static std::vector<FcLight* > mLightsList;
     static std::vector<FcBillboard* > mBillboardsList;
      // NULL DEFAULTS
     // static NullGpu mNullGpu;
     // static NullDescriptor mNullDescriptorClerk;


   public:
      //static void initialize() { pAudioService = & nullAudioService; }
      //static Audio& getAudio() { return *pAudioService; }
     static void init();
     static void provide(FcGpu* gpu);
     static void provide(FcDescriptorClerk* descriptorClerk);
     static void provide(VkExtent2D screenDimensions);
     static void provide(FcRenderer* renderer);
      // - GETTERS -
      // TODO think about making some of these const
     static FcRenderer& Renderer();
     static FcGpu& Gpu() { return *pGpu; }
     static FcDescriptorClerk& DescriptorClerk() { return *pDescriptorClerk; }
     // ?? figure out why this one is const static
     const static VkDevice& Device() { return pDevice; }
     static VkPhysicalDevice vkPhysicalDevice() { return pPhysicalDevice; }
     static VkExtent2D& ScreenDims() { return mScreenDimensions; }
     /* static std::vector<FcGameObject* >& GameObjects() { return mGameObjectsList; } */
     static std::vector<FcLight* >& Lights() { return mLightsList; }
     static std::vector<FcBillboard* >& Billboards() { return mBillboardsList; }
//     static std::vector<FcGameObject*>* GameObjectsTest() { return &mGameObjectsList; }
  };
} // _END_ namespace fc
