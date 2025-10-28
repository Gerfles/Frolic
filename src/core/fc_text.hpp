#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_descriptors.hpp"
#include "fc_billboard_renderer.hpp"
#include "fc_image.hpp"
//#include "core/mesh.h"
//#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <ft2build.h>
#include <vulkan/vulkan_core.h>
#include <unordered_map>
//?? what's up with the following includes
#include FT_FREETYPE_H /* wierd font-lock issue TODO solve issue */
#include FT_GLYPH_H
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <vector>



namespace fc
{
  class FcFont;

  class FcText
  {
   private:
     FcFont* pFont;
     BillboardPushes mTextBoxSpecs;

     id_t mId;
     FcImage mTextImage;
     VkDescriptorSet mDescriptor{nullptr};
      // -
     VkExtent2D writeTextTexture(const std::string& text, float scale, std::vector<VkImageBlit>& blitsList);
   public:
     using id_t = uint32_t;
     using Map = std::unordered_map<id_t, FcText>;
      // - CTORS -
     FcText(FcFont* font) : pFont{font} {};
     FcText(const FcText&) = delete;
     FcText& operator=(const FcText&) = delete;
     FcText(FcText&&) = default;
     FcText& operator=(FcText&&) = default;

     // - GETTERS -
     VkDescriptorSet getDescriptor() { return mDescriptor; }
     // -
     BillboardPushes& Push() { return mTextBoxSpecs; }
     void createTextBox(int x, int y, int width, int height);
     void createText(const std::string& text, int xPos, int yPos, float scale);
     void editText(const std::string newText, int xPos, int yPos, float scale);
     void free();
  };

} // namespace fc _END_
