#pragma once

// libraries
#include "SDL2/SDL_stdinc.h"
#include "core/fc_model.hpp"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
// std
#include <memory>
#include <unordered_map>

namespace fc
{

  struct TransformComponent
  {
     glm::vec3 translation{}; // position offset
     glm::vec3 scale{1.f, 1.f, 1.f};
     glm::vec3 rotation{};

     glm::mat4 mat4();
     glm::mat3 normalMatrix();
  };

  // TODO determine if there's a better way to not pollute global (fc)
    using GameObjectId = uint32_t;


  class FcGameObject
  {
   private:
   // TODO format corectly


     GameObjectId mUniqueId; // object's unique Id to prevent stale handle table lookups
     uint32_t mHandleIndex; // facilitates faster handle creation

     void assignUniqueObjectId();
     void placeInHandleTable();

     friend class FcGameObjectHandle; // access to id and index

   public:
     typedef enum
     {
       UNKNOWN = 0,
       TERRAIN = 1,
       POWER_UP = 2,
       ENEMY = 3,
     } GameObjectType;

     FcGameObject() = delete;
     explicit FcGameObject(FcModel* model, int type = 0);// : pModel(model) { FcGameObject(); }
     FcGameObject(FcModel* model, uint32_t type);
//     using Map = std::unordered_map<id_t, FcGameObject>;
      // - CTORS -
     FcGameObject(const FcGameObject&) = delete;
     FcGameObject& operator=(const FcGameObject&) = delete;
     FcGameObject(FcGameObject&&) = default;
     FcGameObject &operator=(FcGameObject&&) = default;

     uint32_t const Handle() { return mHandleIndex; }
     GameObjectId const Id() { return mUniqueId; }
     glm::vec3 color;
     TransformComponent transform{};
      // optional components

      // std::shared_ptr<FcModel> pModel{};
     FcModel* pModel;

  }; // CLASS FcGameObject

   // ?? not sure I need the handle object for anything TODO - delete maybe
   // define the size of the handle table and hence the maximum number of game objects that can exist simultaneously
  static const Uint32 MAX_GAME_OBJECTS = 2048;

   // wrapper to allow easy lookups etc.
  class FcGameObjectHandle
  {
   private:
     uint32_t mHandleIndex;// index into the table
     GameObjectId mUniqueId; // uniqe id avoids stale handles

   public:
     explicit FcGameObjectHandle(FcGameObject& object) : mHandleIndex(object.mHandleIndex),
                                                         mUniqueId(object.mUniqueId) {}

      // function to dereference the handle
     FcGameObject* toObject() const;
  };


} // NAMESPACE lve
