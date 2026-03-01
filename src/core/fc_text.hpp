//>--- fc_text.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <unordered_map>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcFont; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcText
  {
   private:
     FcFont* pFont;
     id_t mId;
     FcImage mTextImage;
     VkDescriptorSet mDescriptor{nullptr};
     VkExtent2D writeTextTexture(const std::string& text, float scale, std::vector<VkImageBlit>& blitsList);

   public:
     using id_t = uint32_t;
     using Map = std::unordered_map<id_t, FcText>;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcText(FcFont* font) : pFont{font} {};
     FcText(const FcText&) = delete;
     FcText& operator=(const FcText&) = delete;
     FcText(FcText&&) = default;
     FcText& operator=(FcText&&) = default;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MUTATORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void createTextBox(int x, int y, int width, int height);
     void createText(const std::string& text, int xPos, int yPos, float scale);
     void editText(const std::string newText, int xPos, int yPos, float scale);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkDescriptorSet getDescriptor() { return mDescriptor; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void free();
  };

} // namespace fc _END_
