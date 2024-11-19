#include "core/fc_light.hpp"

#include "core/fc_billboard_render_system.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_image.hpp"
#include "core/fc_locator.hpp"
#include "core/utilities.hpp"


namespace fc
{

   // ?? this is not working for some reason
  FcImage FcLight::mPointLightTexture{};//{"plain.png"};
  //FcImage FcLight::mPointLightTexture{"point_light.png"};
  uint32_t FcLight::mTextureId = 0;//mPointLightTexture.loadTexture("point_light.png");

  void FcLight::loadDefaultTexture(std::string filename)
  {
    mTextureId = mPointLightTexture.loadTexture(filename);

//  mBillboard.setTextureId(texID);
//  mPointLightTexture.loadTexture("point_light.png");
  }

   //?? what
  FcLight::FcLight(float intensity, float radius, glm::vec3 color)
    : mBillboard{radius, radius, glm::vec4(color, intensity)}
  {
     //  // ?? TODO not sure if we can initialize a vec4 this way but try
     // mBillboard.Push().color = glm::vec4(color, intensity);
     // mBillboard.Push().width = mBillboard.Push().height = radius;
    placeInHandleTable();
    mBillboard.setTextureId(mTextureId);
    mBillboard.placeInHandleTable();
  }


  // void FcLight::createLight(float intensity, float radius, glm::vec3 color)
  // {
  //   mBillboard.PushComponent().color = glm::vec4(color, intensity);
  //   mBillboard.PushComponent().width = radius;
  //   mBillboard.PushComponent().height = radius;

  //    // BillboardPushComponent push = mBillboard.Push();
  //    // push.color = glm::vec4(color, intensity);
  //    // push.width = radius;
  //    // push.height = radius;

  //   mBillboard.setTextureId(mTextureId);
  //   mBillboard.placeInHandleTable();
  //  }


  FcLight::FcLight()
  {
     // TODO implement defaults if need be
  }


  void FcLight::setPosition(glm::vec3 position)
  {
    mBillboard.PushComponent().position = glm::vec4(position, 1.f);
  }



  glm::vec4& FcLight::getPosition()
  {
    return mBillboard.PushComponent().position;
  }


  PointLight FcLight::generatePointLight()
  {

//    TODO return initialized
    PointLight light;
    light.position = mBillboard.PushComponent().position;
    light.color = mBillboard.PushComponent().color;
    return light;

     //return {mBillboard.PushComponent().position, mBillboard.PushComponent().color};
  }



  void FcLight::placeInHandleTable()
  {
    std::vector<FcLight*>& lightsList = FcLocator::Lights();

     // first check to see if there's already a slot available that's just been set to nullptr
    for(size_t i = 0; i < lightsList.size(); i++)
    {
      if (lightsList[i] == nullptr)
      {
        lightsList[i] = this;
        mHandleIndex = i;
         // don't think we need uniqueId in this handle system since all will be lights

        return;
      }
    }

     // if no slots are vacant, grow the vector of game objects as long as it doesn't exceed the maximum
     // TODO add error code to handle too big of vector
    if (lightsList.size() < MAX_LIGHTS)
    {
      lightsList.push_back(this);
      mHandleIndex = lightsList.size() - 1;
    }
    else
    {
       // if no open slots are found, return the last index, TODO should save last element for special case (ie. invalid index);
      mHandleIndex = MAX_LIGHTS;

       // BUG dangling pointers and such!!!
       // TODO make sure to delete the light since
    }

  }
}
