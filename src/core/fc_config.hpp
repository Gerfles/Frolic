//>--- fc_config.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/log.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <string>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  class FcWindow;
  //
  class FcConfig
  {
   public:
     enum class ColorSpace : uint8_t
     {
       SRGB_NONLINEAR
     , SRGB_EXTENDED_LINEAR
     , HDR10
     };
     ColorSpace requestedColorSpace;
     std::string applicationName;
     u32 appVersionMajor {0};
     u32 appVersionMinor {0};
     u32 appVersionPatch {0};
     u32 windowWidth {0};
     u32 windowHeight {0};
     u32 mouseDeadzone {0};
     bool enableNonUniformScaline {false};
     bool enableDebugShadowmapDraw {false};
     bool isFullscreen {false};
     //
     void enableValidationLayers() noexcept;
     //
     inline void addDeviceExtension(const char* extensionName) noexcept
      {mDeviceExtensions.push_back(extensionName); }
     //
     inline void addInstanceExtension(const char* extensionName) noexcept
      {mInstanceExtensions.push_back(extensionName); }
     //
     inline void addValidationLayer(const char* layerName) noexcept {validationLayers.push_back(layerName); }
     //
     VkLayerSettingsCreateInfoEXT* getValidationLayerSettings();
     //
     void enableBindlessDescriptorIndexing();
     //
     void enableExtenedSwapchainColorSpace();
     //
     inline const bool isExtendedSwapchainColorSpaceEnabled() const noexcept
      { return enableExtendedSwapchainColorSpace; }
     //
     const bool areDeviceFeaturesSupported(VkPhysicalDevice device,
                                           VkPhysicalDeviceFeatures2& requiredFeatures_1_0) noexcept;
     //
     void populateVulkanPhysicalDeviceFeatures(VkPhysicalDeviceFeatures2& requiredFeatures_1_0) noexcept;
     //
     const bool areDeviceExtensionsSupported(VkPhysicalDevice device) noexcept;
     //
     const bool areValidationLayersSupported() noexcept;
     //
     const bool areInstanceExtensionsSupported() noexcept;
     //
     inline const u32 getDeviceExtensionCount() const { return mDeviceExtensions.size(); }
     //
     inline const u32 getInstanceExtensionCount() const { return mInstanceExtensions.size(); }
     //
     inline const u32 getValidationLayerCount() const { return validationLayers.size(); }
     //
     inline const char* const* getDeviceExtensions() const { return mDeviceExtensions.data(); }
     //
     inline const char* const* getInstanceExtensions() const { return mInstanceExtensions.data(); }
     //
     inline const char* const* getValidationLayers() const { return validationLayers.data(); }
     //
     inline void setWindowPtr(FcWindow* windowPtr) { pWindow = windowPtr; }
     //
     inline const FcWindow* getWindowPtr() { return pWindow; }

   private:
     std::vector<const char*> mDeviceExtensions;
     std::vector<const char*> mInstanceExtensions;
     // validation layers
     std::vector<const char*> validationLayers;
     bool validationLayersEnabled {false};
     bool enableExtendedSwapchainColorSpace {false};
     VkValidationFeatureEnableEXT featureEnables[3];
     VkValidationFeaturesEXT features;
     VkLayerSettingEXT layerSettings[3];
     VkLayerSettingsCreateInfoEXT layerSettingsCreate;
     FcWindow* pWindow {nullptr};

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
