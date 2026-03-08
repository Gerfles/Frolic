//>--- fc_config.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <string>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  enum class FeatureType : uint8_t
  {
    ValidationLayer,
    InstanceExtension,
    DeviceExtension,
  };


  //
  struct FcConfig
  {
   public:
     std::string applicationName;
     u32 appVersionMajor {0};
     u32 appVersionMinor {0};
     u32 appVersionPatch {0};
     u32 windowWidth {0};
     u32 windowHeight {0};
     u32 mouseDeadzone {0};
     bool enableNonUniformScaline {false};
     bool enableDebugShadowmapDraw {false};
     bool enableValidationLayers {false};

     void enableBindlessDescriptorIndexing();
     //
     const bool areDeviceFeaturesSupported(VkPhysicalDevice device,
                                           VkPhysicalDeviceFeatures2& requiredFeatures_1_0) noexcept;
     //
     void populateVulkanPhysicalDeviceFeatures(VkPhysicalDeviceFeatures2& requiredFeatures_1_0) noexcept;
     //
     // TODO make static or add suppor for extensions / layers to config
     const bool areExtensionsSupported(std::vector<const char*>& extensionsOrLayers,
                                       FeatureType type,
                                       VkPhysicalDevice device = nullptr) noexcept;
   private:
     // Vulkan 1.0 features
     static constexpr bool samplerAnisotropy {VK_TRUE};
     static constexpr bool shaderStorageImageMultisample {VK_TRUE};
     static constexpr bool sampleRateShading {VK_TRUE};
     static constexpr bool geometryShader {VK_TRUE};
     static constexpr bool tessellationShader {VK_TRUE};
     static constexpr bool fillModeNonSolid {VK_TRUE};
     // Vulkan 1.1 features
     //
     // Vulkan 1.2 features
     static constexpr bool descriptorBindingSampledImageUpdateAfterBind {VK_TRUE};
     static constexpr bool descriptorBindingStorageImageUpdateAfterBind {VK_TRUE};
     static constexpr bool shaderSampledImageArrayNonUniformIndexing {VK_TRUE};
     static constexpr bool bufferDeviceAddress {VK_TRUE};
     static constexpr bool descriptorIndexing {VK_TRUE};
     // enables bindless rendering so that we can automatically bind
     // an array of textures that can be used across multiple shaders and accessed
     // via index (note that this is (NOT??) an avaialable feature of the Nintendo Switch)
     // Gives us the ability to partially bind a descriptor, since some entries in the
     // bindless array will need to be empty
     static constexpr bool descriptorBindingPartiallyBound {VK_TRUE};
     // Gives us the bindless descriptor usage with SpirV
     static constexpr bool runtimeDescriptorArray {VK_TRUE};
     // Vulkan 1.3 features
     static constexpr bool dynamicRendering {VK_TRUE};
     static constexpr bool synchronization2 {VK_TRUE};
  };


} // --- namespace fc --- (END)
