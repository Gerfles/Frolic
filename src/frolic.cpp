#include "frolic.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_cvar_system.hpp"
#include "core/fc_font.hpp"
#include "core/fc_light.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_player.hpp"
#include "core/fc_text.hpp"
#include "core/utilities.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_version.h"
#include "SDL2/SDL_video.h"
//
#include "imgui.h"
#include "imgui_impl_sdl2.h"
//
#include "vulkan/vulkan_core.h"
#include <SDL_log.h>
#include <SDL_timer.h>
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>


namespace fc
{

  // float CVar


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

    VkExtent2D screenDims {1400, 1000};
    FcLocator::provide(screenDims);
    // Version info
    //TODO should do this in a builder class that goes out of scope when no longer needed
    SDL_version compiled;
    SDL_VERSION(&compiled);

    SDL_version linked;
    SDL_GetVersion(&linked);

    SDL_Log("SDL version(s): %u.%u.%u (compiled),  %u.%u.%u. (linked)\n",
            compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);
    SDL_Log("Window Dimensions: %i x %i", screenDims.width, screenDims.height);

    // Initialize the memory allocator
    // TODO make this part of our FcLocator scheme instead of the singleton it is
    MemoryService::instance()->init(nullptr);
    pAllocator = &MemoryService::instance()->systemAllocator;
    // TODO Make sure this deallocates properly
    mStackAllocator.init( megabytes(8) );

    // TODO pull some stuff out of render initialize and have init VK systems?
    // TODO define our own exception classes and failure codes for debugging later
    if (mRenderer.init(appInfo, screenDims, &pSceneData) != EXIT_SUCCESS)
    {
      mShouldClose = true;
      return;
    }

    // TODO set this as a static variable
    mInput.init(mRenderer.Window());

    // NOTE: must use screen dimension not pixel width and height
    // TODO should make a better distinction and perhaps have it available globally
    mInput.setMouseDeadzone(50, screenDims.width, screenDims.height);

    mPlayer.init(&mInput);

    // Initialize simple first person camera
    mPlayer.Camera().setPerspectiveProjection(70.0f, FcLocator::ScreenDims().width
                                             , FcLocator::ScreenDims().height, 512.f, 0.1f);
    // TODO make sure all reference returns are const to avoid something like:
    // mPlayer.Camera().Projection()[1][1] *= -1;

    mRenderer.setActiveCamera(&mPlayer.Camera());
    mRenderer.initDefaults();//mSceneDataBuffer, &mSceneData);

    stats = &mRenderer.getStats();

  }// --- Frolic::Frolic (_) --- (END)

  //
  //
  void Frolic::loadGameObjects()
  {
    // TODO research why there seems to be no constructor for a ::path from const char*
    std::filesystem::path filename = "..//textures//sun.png";
    mSunBillboard.loadTexture(filename);
    mSunBillboard.setPosition(pSceneData->sunlightDirection);
    mRenderer.addBillboard(mSunBillboard);

    // TODO implement with std::optional
    helmet.loadGltf(mRenderer, "..//models//helmet//DamagedHelmet.gltf");
    sponza.loadGltf(mRenderer, "..//models//sponza//Sponza.gltf");
    // structure.loadGltf(mSceneRenderer, "..//models//MosquitoInAmber.glb");
    // structure.loadGltf(mSceneRenderer, "..//models//MaterialsVariantsShoe.glb");
    // structure2.loadGltf(this, "..//models//Box.gltf");
    // structure2.loadGltf(this, "..//models//GlassHurricaneCandleHolder.glb");
    // structure2.loadGltf(mSceneRenderer, "..//models//ToyCar.glb");
    // structure2.loadGltf(mSceneRenderer, "..//models//structure.glb");

    // Posistion the loaded scenes
    glm::vec3 translationVec = {45.0f, 9.0f, 20.0f};
    helmet.translate(translationVec);
    translationVec.y -= 2.0f;
    sponza.translate(translationVec);

    // update the moved objects
    helmet.update();
    sponza.update();

  } // --- Frolic::loadGameObjects (_) --- (END)



  void Frolic::run()
  {
    // FIXME if possible, may be an issue with linux
    SDL_ShowCursor(SDL_DISABLE);

    // Initialize player controls and position
    mPlayer.setPosition(glm::vec3(36.f, 25.f, 19.f));

    // load everything we need for the scene
    loadGameObjects();

    AutoCVarFloat cvarMovementSpeed("movementSpeed.float", "controls camera movement speed", 8.0f);


    // zero out the ticklist for performance tracking
    std::memset(mFrameTimeList, 0, sizeof(mFrameTimeList));

    FcTimer mTimer;
    mTimer.start();
    float deltaTime = 0.0f;

    // TODO separate to make entirely own function
    while (!mShouldClose)
    {
      bool shouldResize = false;
      // Check for events every cycle of the game loop
      while (SDL_PollEvent(&mEvent))
      {
        if (mEvent.type == SDL_WINDOWEVENT)
        {
          // TODO should think about updating input here instead of doing it asynchronously...
          switch (mEvent.window.event)
          {
            // ?? check that this is proper
              case SDL_WINDOWEVENT_MINIMIZED:
              case SDL_WINDOWEVENT_HIDDEN:
              {
                // TODO replace here with mRenderer.pause();
                SDL_WaitEvent(nullptr);
                break;
              }
              // ?? this event seems to cause issues in Wayland, seems like wayland already handles resize
              // case SDL_WINDOWEVENT_SIZE_CHANGED:
              // {
              //    // TODO handle better here

              //   mRenderer.handleWindowResize();
              //   shouldResize = true;
              //    //break;
              // }
              default:
                break;
          }
        }

        if (mEvent.type == SDL_QUIT)
        {
          mShouldClose = true;
          break;
        }

        // TODO see if this is better handles (without this additional check) from SDL switch
        if (mRenderer.shouldWindowResize())
        {
          mRenderer.handleWindowResize();
        }

        // Send SDL event to imGUi for handling
        ImGui_ImplSDL2_ProcessEvent(&mEvent);
      }
      // TODO consider creating a timer that better represents time in seconds instead of milliseconds
      // that way we don't need to divide by 1000 to get a better representation of what's going on...
      // could consider bit-shifting by 1024 to get there also
      deltaTime = mTimer.elapsedTime();
      /* FcStats& stats = mRenderer.getStats(); */
      stats->frametime = deltaTime * 1000;
      stats->fpsAvg = calcFPS(deltaTime);
      // now re-start the time so that the start time is the start of each frame
      mTimer.start();

      // update keyboard and game controller state and use that to mover the mPlayer (and thus camera)
      mInput.update();

      // Move the mPlayer with the updated input, which will automatically update the camera
      mPlayer.move(deltaTime);

      update(deltaTime);


      // pSceneData->viewProj = pSceneData->projection * pSceneData->view;
      // pSceneData->lighSpaceTransform = mRenderer.mShadowMap.LightSpaceMatrix();
// mSceneDataBuffer.overwriteData(&mSceneData, sizeof(SceneData));

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   START THE NEW FRAME   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // mRenderer.generateShadowMap();

      uint32_t swapchainImgIndex = mRenderer.beginFrame();

      // FcPipeline* selected = mPipelines[currentBackgroundEffect];
      /* mRenderer.attachPipeline(selected); */

      // TODO make sure we can comment out drawGUI without crashing
      drawGUI();

      //mRenderer.drawModels(swapchainImgIndex, mUbo);
      //mRenderer.drawBillboards(camera.Position(), frame, mUbo);
      //mRenderer.drawUI(mUItextList, frame);
      //mRenderer.drawBackground(mPushConstants[currentBackgroundEffect]);

      // TODO may want to couple shadow map tighter with sceneRenderer
      mRenderer.drawFrame(mDebugShadowMap);

      mRenderer.endFrame(swapchainImgIndex);

    } // _END_ while(!shouldClose);
  }


  void Frolic::update(float deltaTime)
  {
    angle = rotationSpeed * 0.001f;
    glm::vec3 rotationAxis = {0.f, -1.f, 0.f};

    helmet.rotateInPlace(angle, rotationAxis);
    /* structure2.rotateInPlace(angle, rotationAxis); */

    // TODO could flag draw collection items with nodes that "know" they need to be updated
    // then draw collection could do all the updating in one go.
    helmet.update();
    /* structure2.update(mDrawCollection); */
  }



  void Frolic::drawGUI()
  {
    // test ImGui UI
    // Left here to add a demo windo that names all the features for (handy for searching)
    /* ImGui::ShowDemoWindow(); */

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   STATISTICS WINDOW   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    if (ImGui::Begin("Frolic Stats", NULL, ImGuiWindowFlags_NoTitleBar))
    {
      // Stats
      ImGui::Text("FPS(avg): %i ", stats->fpsAvg);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Frame(ms): %.2f", stats->frametime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Draw(ms): %.2f", stats->meshDrawTime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Update(ms): %.2f", stats->sceneUpdateTime);
      ImGui::SameLine(0.f, 10.f);

      // TODO UNCOMMENT
      // TODO should probably have a Position() method in FcPlayer
      glm::vec3 pos = mPlayer.Camera().Position();
      ImGui::Text("| Position: <%.3f,%.3f,%.3f>", pos.x, pos.y, pos.z);
      ImGui::SameLine(0.f, 10.f);

      ImGui::Text("| Triangles Drawn: %i", stats->triangleCount);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Objects Drawn: %i", stats->objectsRendered);
      // int relX, relY;
      // mInput.RelativeMousePosition(relX, relY);
      // ImGui::Text("Mouse X pos: %i", mInput.getMouseX());
      // ImGui::Text("Mouse Y pos: %i", mInput.getMouseY());

      // TODO check the official way to use ImGui via the imgui example source code
      ImGui::End();
    }

    if (mPlayer.lookSpeed() == 0.0f)
    {
      // TODO probably best to enable disable this stuff eventually in a dedicated pipeline shader
      // and then just bind the appropriate pipeline
      // Draw Configuration panel
      if (ImGui::Begin("Scene Data"))
      {
        // TODO update all options with bitfields instead of bools

        if (ImGui::Checkbox("Wire Frame", &mRenderer.drawWireframe))
        {
          // b
        }

        if (ImGui::Checkbox("Color Texture", &mUseColorTexture))
        {
          helmet.toggleTextureUse(MaterialFeatures::HasColorTexture, helmetTexIndices);
          sponza.toggleTextureUse(MaterialFeatures::HasColorTexture, sponzaTexIndices);
        }

        if (ImGui::Checkbox("Rough/Metal Texture", &mUseRoughMetalTexture))
        {
          helmet.toggleTextureUse(MaterialFeatures::HasRoughMetalTexture, helmetTexIndices);
          sponza.toggleTextureUse(MaterialFeatures::HasRoughMetalTexture, sponzaTexIndices);
        }

        if(ImGui::Checkbox("Ambient Occlussion Texture", &mUseOcclussionTexture))
        {
          helmet.toggleTextureUse(MaterialFeatures::HasOcclusionTexture, helmetTexIndices);
          sponza.toggleTextureUse(MaterialFeatures::HasOcclusionTexture, sponzaTexIndices);
        }

        if (ImGui::Checkbox("Normal Texture", &mUseNormalTexture))
        {
          helmet.toggleTextureUse(MaterialFeatures::HasNormalTexture, helmetTexIndices);
          sponza.toggleTextureUse(MaterialFeatures::HasNormalTexture, sponzaTexIndices);
        }

        if (ImGui::Checkbox("Emissive Texture", &mUseEmissiveTexture))
        {
          helmet.toggleTextureUse(MaterialFeatures::HasEmissiveTexture, helmetTexIndices);
          sponza.toggleTextureUse(MaterialFeatures::HasEmissiveTexture, sponzaTexIndices);
        }

        ImGui::Checkbox("Draw Normals", &mRenderer.mDrawNormalVectors);

        ImGui::Checkbox("Box Bounds", &mRenderer.mDrawBoundingBoxes);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);

        // FIXME maybe create a struct of variables that we then pass
        // as a whole to mRenderer...
        if (ImGui::InputInt("Bounding Box", &mRenderer.mBoundingBoxId))
        {
          if (mRenderer.mBoundingBoxId < 0)
            mRenderer.mBoundingBoxId = -1;
        }

        ImGui::SetNextItemWidth(60);
        ImGui::SliderInt("Model Rotation Speed", &rotationSpeed, -5, 5);
        ImGui::SetNextItemWidth(60);

        // TODO this shouldn't be a CVAR but left for reference
        float* movementSpeed = CVarSystem::Get()->GetFloatCVar("movementSpeed.float");
        if (ImGui::SliderFloat("Movement Speed", movementSpeed, 1, 50, "%1.f"))
        {
          mPlayer.setMoveSpeed(*movementSpeed);
        }

        ImGui::Checkbox("Cycle", &mCycleExpansion);

        if (mCycleExpansion)
        {
          float time = SDL_GetTicks() / 1000.0f;

          // FIXME
          mRenderer.ExpansionFactor() = sin(time) + 1.0f;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);

        // FIXME
        ImGui::SliderFloat("Expansion Factor", &mRenderer.ExpansionFactor(), -1.f, 2.f);

        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SUNLIGHT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        glm::vec4 sunlightPos = pSceneData->sunlightDirection;
        if (ImGui::SliderFloat("Sun X", &sunlightPos.x, -100.f, 100.f)
            || ImGui::SliderFloat("Sun Y", &sunlightPos.y, 5.f, 100.f)
            || ImGui::SliderFloat("Sun Z", &sunlightPos.z, -100.f, 100.f))
        {
          // Update sun's location
          mSunBillboard.setPosition(sunlightPos);
          pSceneData->sunlightDirection = sunlightPos;

          // Update shadow map light source
          glm::vec3 lookDirection{sunlightPos.x, 0.f, sunlightPos.z};
          mRenderer.mShadowMap.updateLightSource(sunlightPos, lookDirection);
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW MAP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // TODO use frustum instead
        Box& frustum = mRenderer.mShadowMap.Frustum();
        ImGui::Checkbox("Draw Shadow Map", &mDebugShadowMap);
        if(ImGui::SliderFloat("Left", &frustum.left, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Right", &frustum.right, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Top", &frustum.top, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Bottom", &frustum.bottom, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Near", &frustum.near, -.01f, 75.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Far", &frustum.far, 1.f, 100.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-   VIEW MATRIX COMPARISON   -*-*-*-*-*-*-*-*-*-*-*-*- //
        if (ImGui::Button("Display View Matrix"))
        {
          ImGui::OpenPopup("MatrixView");
        }
        if (ImGui::BeginPopup("MatrixView"))
        {
          glm::mat4 mat = mPlayer.Camera().getViewMatrix();
          /* glm::mat4 mat2 = uvnPlayer.Camera().getViewMatrix(); */

          ImGui::Text("Quaternion View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
          // ImGui::Text("UVN View Matrix");
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][0], mat2[1][0], mat2[2][0], mat2[3][0]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][1], mat2[1][1], mat2[2][1], mat2[3][1]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][2], mat2[1][2], mat2[2][2], mat2[3][2]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][3], mat2[1][3], mat2[2][3], mat2[3][3]);
          ImGui::EndPopup();
        }
      }
      // ??
      //ImGui::EndFrame();
      ImGui::End();
    }
    // make ImGui calculate internal draw structures
    ImGui::Render();
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

     // TODO eventually improve on this metric
    if (mFrameTimeIndex == MAX_FRAME_SAMPLES - 1)
    {
      float avgFrameTime = 0.0f;

      // Start at one so we don't count the first frame
      for (int i = 1; i < MAX_FRAME_SAMPLES; ++i)
      {
        avgFrameTime += mFrameTimeList[i];
      }

      avgFrameTime = (MAX_FRAME_SAMPLES - 1)/avgFrameTime;

       //      std::cout << "average frames per second: " << avgFrameTime << std::endl;
    }

     // make sure we wrap around on the MAX_FRAME_SAMPLE'th sample
    mFrameTimeIndex = (mFrameTimeIndex + 1) % MAX_FRAME_SAMPLES;

     // calculate and return the average famesPerSec
    return MAX_FRAME_SAMPLES / mFrameTimeSum;
  }



  void Frolic::close()
  {
    // TODO I think that since we have the one device wait idle, we can eliminate all others
    vkDeviceWaitIdle(FcLocator::Device());

    helmet.clearAll();
    helmet.destroy();
    sponza.clearAll();
    sponza.destroy();

    mInput.kill();

     // free image resources
    mFallbackTexture.destroy();
    //
    mMeshPipeline.destroy();

    FcLight::destroyDefaultTexture();

     // free all bilboards
    for (auto& text : mUItextList)
    {
      text.free();
    }

    mUIfont.free();

    mGradientPipeline.destroy();
    mSkyPipeline.destroy();
     // for (auto& model : mModelList)
     // {
     //   model.destroy();
     // }

    mRenderer.shutDown();

    MemoryService::instance()->shutdown();
  }

} // namespace fc END
