#pragma once

#include "core/fc_image.hpp"
#include "core/fc_billboard_render_system.hpp"
#include "core/fc_game_object.hpp"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


namespace fc
{

  struct PointLight
  {
      // TODO w used for point light or directional
     glm::vec4 position {0.f}; //  ignore w
     glm::vec4 color {1.f}; // w is intensity
  };


    class FcLight
    {
     private:
       static FcImage mPointLightTexture;
       static uint32_t mTextureId;
       FcBillboard mBillboard;
        // TODO w of position used for point light or directional
        // TODO thing about geetting rid of mTransform
       TransformComponent mTransform;
       uint32_t mHandleIndex; // ?? Not sure we need a handle into the index
       void placeInHandleTable();
     public:
       FcLight(float intensity, float radius, glm::vec3 color);
       FcLight();

       static void loadDefaultTexture(std::string filename);
       static void destroyDefaultTexture() { mPointLightTexture.destroy(); }
        //void createLight(float intensity, float radius, glm::vec3 color);

       void setPosition(glm::vec3 position);
       glm::vec4& getPosition();

        //
       PointLight generatePointLight();
        //
       TransformComponent& Transform() { return mTransform; }
    };


}
