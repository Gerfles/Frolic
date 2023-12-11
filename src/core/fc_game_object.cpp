#include "fc_game_object.hpp"
#include "core/fc_locator.hpp"
#include "core/utilities.hpp"
#include <_types/_uint32_t.h>
#include <memory>
#include <sys/_types/_size_t.h>
#include <vector>


namespace fc
{

  FcGameObject::FcGameObject(FcModel* model, int type) : pModel(model)
  {
     //pModel = nullptr;

     // the unique id might come from the world editor or it might be assigned dynamically at runtime

     //assignUniqueObjectId();
    mUniqueId = type;
     // the handle index is assigned by finding the first free slot in the handle table
    placeInHandleTable();
  }
  

  // FcGameObject::FcGameObject()
  // {
  //    //pModel = nullptr;

  //    // the unique id might come from the world editor or it might be assigned dynamically at runtime
  //   assignUniqueObjectId();

  //    // the handle index is assigned by finding the first free slot in the handle table
  //   placeInHandleTable();

  // }



  FcGameObject::FcGameObject(FcModel* model, uint32_t type) : pModel(model)
  {
  }



   // TODO actually implement or get rid of this function
   // the unique id might come from the world editor or it might be assigned dynamically at runtime
  void FcGameObject::assignUniqueObjectId()
  {
    // if (type == TERRAIN)
    // {
      
    // }
    // else if (type == UNKNOWN)
  }



  
  void FcGameObject::placeInHandleTable()
  {
    std::vector<FcGameObject*>& gameObjectsList = FcLocator::GameObjects();

    // first check to see if there's already a slot available that's just been set to nullptr
    for(size_t i = 0; i < gameObjectsList.size(); i++)
    {
      if (gameObjectsList[i] == nullptr)
      {
        gameObjectsList[i] = this;
        mHandleIndex = i;
        return;
      }
    }

     // if no slots are vacant, grow the vector of game objects as long as it doesn't exceed the maximum
     // TODO add error code to handle too big of vector
    if (gameObjectsList.size() < MAX_GAME_OBJECTS)
    {
      gameObjectsList.push_back(this);
      mHandleIndex = gameObjectsList.size() - 1;
    }
    else
    {
       // if no open slots are found, return the last index, TODO should save last element for special case (ie. invalid index);
       // BUG dangling pointers and such!!!
       // TODO make sure to delete the light since
      mHandleIndex = MAX_GAME_OBJECTS;
    }
  }




// Matrix corresponds to translate * Roty * Rotx * Rotz * scale transformation
   // Rotation convention uses tait-bryan (euler) angles with axis order Y(1), X(2), Z(3)
   // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
  glm::mat4 TransformComponent::mat4()
  {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);

    return glm::mat4{
      {
        scale.x * (c1 * c3 + s1 * s2 * s3),
            scale.x * (c2 * s3),
            scale.x * (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
            scale.y * (c3 * s1 * s2 - c1 * s3),
            scale.y * (c2 * c3),
            scale.y * (c1 * c3 * s2 + s1 * s3),
            0.0f,
        },
        {
            scale.z * (c2 * s1),
            scale.z * (-s2),
            scale.z * (c1 * c2),
            0.0f,
        },
        {translation.x, translation.y, translation.z, 1.0f}};
  }

  
  
  glm::mat3 TransformComponent::normalMatrix()
  {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 invScale = 1.0f / scale;

    return glm::mat3{
      {
        invScale.x*(c1 * c3 + s1 * s2 * s3),
	invScale.x * (c2 * s3),
        invScale.x * (c1 * s2 * s3 - c3 * s1),
      },
      {
        invScale.y * (c3 * s1 * s2 - c1 * s3),
        invScale.y * (c2 * c3),
        invScale.y * (c1 * c3 * s2 + s1 * s3),
      },
      {
        invScale.z * (c2 * s1),
        invScale.z * (-s2),
        invScale.z * (c1 * c2),
      }
    };
  } /// ::normalMatrix()

  

  // FcGameObject FcGameObject::createGameObject()
  // {
  //   static id_t currentId = 0;
  //   return FcGameObject{currentId++};
  // }



  FcGameObject* FcGameObjectHandle::toObject() const
  {
    // FcGameObject* pGameObject = FcLocator::GameObjects()[mHandleIndex];

    // if (pGameObject != nullptr && pGameObject->mUniqueId == mUniqueId)
    // {
    //   return pGameObject;
    // }

    return nullptr;
  }
  
} /// NAMESPACE lve ///
