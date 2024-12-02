#include "frolic.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_descriptors.hpp"
#include "core/fc_game_object.hpp"
#include "core/fc_font.hpp"
#include "core/fc_light.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_model.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_version.h"
#include "SDL2/SDL_video.h"
#include "core/fc_player.hpp"
#include "core/fc_text.hpp"
#include "core/utilities.hpp"
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>


namespace fc
{
  Frolic::Frolic()
  {
     // Application Specs for developer use
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Frolic"; 		   // Our application name
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // our application version
    appInfo.pEngineName        = "Frolic Engine";          // custom engine name
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0); // custom engine version
     // the following does affect the program unlike the above
    appInfo.apiVersion         = VK_API_VERSION_1_3;       // the version of Vulkan ()

    VkExtent2D screenDims {1200, 900};
    FcLocator::provide(screenDims);
     // Version info
     //TODO should do this in a builder class that goes out of scope when no longer needed
    SDL_version compiled;
    SDL_VERSION(&compiled);

    SDL_version linked;
    SDL_GetVersion(&linked);

    SDL_Log("SDL version(s): %u.%u.%u (compiled),  %u.%u.%u. (linked)\n",
            compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);

     //TODO should maybe define our own exit_success and failure codes for debugging later
    if (mRenderer.init(appInfo, screenDims) != EXIT_SUCCESS)
    {
      mShouldClose = true;
    }

     // zero out the ticklist for performance tracking
    std::memset(mFrameTimeList, 0, sizeof(mFrameTimeList));

     // create a default texture that gets used whenever assimp cant find a texture
     // Need to load this in first so it can be Texture 0 for default
     // TODO set this as a static variable
    mFallbackTexture.loadTexture("plain.png");
    FcLight::loadDefaultTexture("point_light.png");

    mInput.init();
  }



  void Frolic::loadUIobjects()
  {
     // add a model to the model list
     // TODO make sure move constructor exists
    mUIfont.createRasterFont("fonts/Arial.ttf", 25, 32, 122);
    FcText text{&mUIfont};
    text.createText("FPS:", 10, 15, 1.0f);
    mUItextList.emplace_back(std::move(text));

    text.createText("9999", 70, 16, 1.0f);
    mUItextList.emplace_back(std::move(text));
  }



  void Frolic::loadGameObjects()
  {
     // Load the castle object
    FcModel* model = new FcModel{"models/castle.obj"};
    FcGameObject* castle =  new FcGameObject(model, FcGameObject::POWER_UP);
    castle->transform.rotation = {glm::pi<float>(), 0.f, 0.f};
    castle->transform.translation = {0.f, 1.f, 0.f};
    castle->transform.scale = {0.5f, 0.5f, 0.5f};

     // load the vase object
    // model = new FcModel{"models/smooth_vase.obj"};
    // FcGameObject* vase = new FcGameObject(model, FcGameObject::POWER_UP);
    // vase->transform.translation = { 1.2f, 0.8f, 0.f };
    // vase->transform.scale = { 3.f, 3.f, 3.f };

     // load the floor
    model = new FcModel{"models/quad.obj"};
    FcGameObject* floor = new FcGameObject(model, FcGameObject::TERRAIN);
    floor->transform.translation = { 0.f, 1.f, 0.f };
    floor->transform.scale = { 8.f, 8.f, 8.f };

     // load the point lights
    std::vector<glm::vec3> lightColors{ {1.f, .1f, .1f},
                                        {.1f, .1f, 1.f},
                                        {.1f, 1.f, .1f},
                                        {1.f, 0.f, 1.f},
                                        {1.f, 1.f, 1.f},
                                        {0.f, 1.f, 1.f}};

    for (int i = 0; i < lightColors.size(); i++)
    {

      FcLight* light = new FcLight(1.0f, 0.044f, lightColors[i]);

      auto rotateLight = glm::rotate( glm::mat4(1.f), i * glm::two_pi<float>() / lightColors.size()
                                      , {0.f, -1.f, 0.f});

      light->setPosition(glm::vec3(rotateLight * glm::vec4(-1.f, -1.3f, 2.3f, 0.f)));

      mUbo.pointLights[i] = light->generatePointLight();
    }

    mUbo.numLights = lightColors.size();


     // add the sun
    FcLight* sun = new FcLight(7.0f, 0.4f, {1.0f, 1.0f, 1.0f});
    sun->setPosition({0.0f, -4.5f, 0.0f});
    mUbo.pointLights[mUbo.numLights] = sun->generatePointLight();
    mUbo.numLights += 1;
  }



  void Frolic::run()
  {
    pLog("run()");

    float deltaTime = 0.0f;

    mTimer.start();

    FcPlayer player{mInput};
    player.setPosition(glm::vec3(0.f, 0.f, -5.f));
     // Simple first person camera
    FcCamera camera;

    camera.setPerspectiveProjection(glm::radians(45.0f), mRenderer.AspectRatio(), 0.1f, 100.f);
    mUbo.projection = camera.Projection();

    camera.setViewDirection(glm::vec3(0.f, 0.f, -5.f), glm::vec3(0.f, 0.f, 0.f));

    FcCamera testCamera = camera;

     // load everything we need for the scene
    loadUIobjects();
    loadGameObjects();

     // TODO this should be abstracted into engine
    while (!mShouldClose)
    {
       // Check for events every cycle of the game loop
      while (SDL_PollEvent(&mEvent))
      {
        if (mEvent.type == SDL_WINDOWEVENT)
        {
          switch (mEvent.window.event)
          {
             // ?? Not sure what the difference is...
              case SDL_WINDOWEVENT_MINIMIZED:
              {
                SDL_WaitEvent(nullptr);
                break;
              }
              case SDL_WINDOWEVENT_SIZE_CHANGED:
              {
                mRenderer.handleWindowResize(); // ?? NOTE: vulkan tutorial uses a bool flag here instead of calling function directly
                break;
              }
              default:
                break;
          }
        }

        if (mEvent.type == SDL_QUIT)
        {
          mShouldClose = true;
          break;
        }
      } // END of Events

       // TODO consider creating a timer that better represents time in seconds instead of milliseconds
       // that way we don't need to divide by 1000 to get a better representation of what's going on...
       // could consider bit-shifting by 1024 to get there also
      deltaTime = mTimer.elapsedTime();

       // update keyboard and game controller state and use that to mover the player (and thus camera)
      mInput.update();
      player.move(deltaTime, camera);

       // now re-start the time so that the start time is the start of each frame
      mTimer.start();

      update(deltaTime);

      uint32_t frame = mRenderer.beginFrame();

      mUbo.view = camera.View();
      mUbo.projection = camera.Projection();
      mUbo.invView = camera.InverseView();

      mRenderer.drawModels(frame, mUbo);

      mRenderer.drawBillboards(camera.Position(), frame, mUbo);

       //  std::cout << fpsString + fps;

      mRenderer.drawUI(mUItextList, frame);
      mRenderer.endFrame(frame);

       // TODO implement better way to wait for queues to finish
      vkDeviceWaitIdle(FcLocator::Device());

    } // _END_ while(!shouldClose);
  }


  void Frolic::update(float deltaTime)
  {
     //assert(modelId < mModelList.size() && modelId >= 0 && "modelID corresponds to non-existent model!");

    static float angle = 0.0f;
     // do program specific stuff here
    angle += 10.0f * deltaTime;
    if (angle > 360.0f)
    {
      angle -= 360.0f;
    }


    glm::mat4 rotateMat = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

     //TODO get rid of and just rotate camera
     rotateMat = glm::rotate(rotateMat, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

     // for (int modelId = 0; modelId < mModelList.size(); ++modelId)
     // {
     //   mModelList[modelId].setModelMatrix(testMat);
     // }

    auto gameObjects = FcLocator::GameObjects();

    for (FcGameObject* gameObject : gameObjects)
    {
       // TODO eliminate this check and just iterate over valid pointers
//      if (gameObject != nullptr)
      if (gameObject->Id() != FcGameObject::TERRAIN)
      {
        glm::vec3 rotate{0.f, -0.5f, 0.f};
         //   mModelList[modelId].setModelMatrix(testMat);
         //gameObject->transform.rotation += deltaTime * rotate;
//        gameObject->pModel->setModelMatrix(testMat);
         //printMat(gameObject->pModel->ModelMatrix());
      }
    }


     //      auto rotateLight = glm::rotate( glm::mat4(1.f), i * glm::two_pi<float>() / lightColors.size()
     //                       , {0.f, -1.f, 0.f});

     //    mUbo.pointLights[i].setPosition(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f)));


     //light->setPosition(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f)));
     //mLights.back().setPosition(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f)));

     //
    rotateMat = glm::rotate(glm::mat4(1.f), glm::radians(25 * deltaTime), {0.f, -1.f, 0.f});


    auto lights = FcLocator::Lights();
     //
    for (size_t i = 0; i < lights.size() - 1; ++i)
    {

       //light->Transform().translation = glm::vec3(rotateMat * glm::vec4(light->Transform().translation, 1.f));

      lights[i]->setPosition(rotateMat * lights[i]->getPosition());

       // copy light to ubo
      mUbo.pointLights[i] = lights[i]->generatePointLight();
    }

     // TODO put timer and FPS stuff in Renderer
     // use the timers elapsed time to calculate the average frames per second
     // PROFILING
    int avgFPS  = calcFPS(deltaTime);
    std::string fpsNum{std::to_string(avgFPS)};

     // TODO editing this text costs about 30-40 fps for some reason, fix
     mUItextList[1].editText(fpsNum, 70, 16, 1.0f);

     // if (mFrameTimeIndex == 0)
     // {
     //   std::cout << "FPS: " << avgFPS << std::endl;
     // }

     // mPipeline.updateUniformBuffer(imgIndex, &mUboViewProjection);
     // font.setModelMatrix(newModelMatrix);
  }


   // Keep a running total of 100 frame time samples in order to smooth the FPS calculation
  int Frolic::calcFPS(float lastFrameTime)
  {
     // first remove the frame time at the current index from our running total
    mFrameTimeSum -= mFrameTimeList[mFrameTimeIndex];

     // then add the most recent frame time to our frame time total
    mFrameTimeSum += lastFrameTime;

     // store the most recent frame in our array so we can subtract it from the total the next time around
    mFrameTimeList[mFrameTimeIndex] = lastFrameTime;

     // TODO eventually get rid of this metric
    if (mFrameTimeIndex == MAX_FRAME_SAMPLES - 1)
    {
      float avgFrameTime = 0.0f;

      for (int i = 0; i < MAX_FRAME_SAMPLES; ++i)
      {
        avgFrameTime += mFrameTimeList[i];
      }

      avgFrameTime = MAX_FRAME_SAMPLES/avgFrameTime;

       //      std::cout << "average frames per second: " << avgFrameTime << std::endl;
    }

     // make sure we wrap around on the MAX_FRAME_SAMPLE'th sample
    mFrameTimeIndex = (mFrameTimeIndex + 1) % MAX_FRAME_SAMPLES;

     // calculate and return the average famesPerSec
    return MAX_FRAME_SAMPLES / mFrameTimeSum;
  }




  void Frolic::close()
  {
    mInput.kill();

     // TODO I think that since we have the one device wait idle, we can eliminate all others
    vkDeviceWaitIdle(FcLocator::Device());

     // free image resources
    mFallbackTexture.destroy();

    FcLight::destroyDefaultTexture();

    for(auto& gameObject : FcLocator::GameObjects())
    {
      gameObject->pModel->destroy();
      delete gameObject->pModel;
      delete gameObject;
    }


     // free all bilboards
    for (auto& text : mUItextList)
    {
      text.free();
    }

    mUIfont.free();

     // for (auto& model : mModelList)
     // {
     //   model.destroy();
     // }

    mRenderer.shutDown();
  }

} // namespace fc END
