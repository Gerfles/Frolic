#pragma once

// - FROLIC ENGINE -
//#include "core/fc_descriptors.hpp"
#include "fc_billboard_render_system.hpp"
#include "fc_image.hpp"
//#include "core/mesh.h"
//#include "fc_pipeline.hpp"
// - EXTERNAL LIBRARIES -
#include "glm/glm.hpp"
#include <ft2build.h>
#include <unordered_map>
//?? what's up with the following includes
#include FT_FREETYPE_H /* wierd font-lock issue TODO solve issue */
#include FT_GLYPH_H
// - STD LIBRARIES -
#include <string>



namespace fc
{
  class FcFont;

  class FcText
  {
   private:
     FcFont* pFont;
     BillboardPushComponent mTextBoxSpecs;

      // TODO delete one of
     uint32_t mTextureId;
     id_t mId;
     FcImage mTextImage;

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
     uint32_t TextureId() const { return mTextureId; }

     // -
     BillboardPushComponent& Push() { return mTextBoxSpecs; }
     void createTextBox(int x, int y, int width, int height);
     void createText(const std::string& text, int xPos, int yPos, float scale);
     void editText(const std::string newText, int xPos, int yPos, float scale);
     void free();
  };

} // namespace fc _END_
