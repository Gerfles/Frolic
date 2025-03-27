#include "frolic.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_descriptors.hpp"
#include "core/fc_game_object.hpp"
#include "core/fc_font.hpp"
#include "core/fc_light.hpp"
#include "core/fc_locator.hpp"
//#include "core/fc_mesh.hpp"
#include "core/fc_model.hpp"
//#include "core/fc_pipeline.hpp"
#include "core/fc_player.hpp"
#include "core/fc_text.hpp"
#include "core/utilities.hpp"
//#include "core/fc_timer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_version.h"
#include "SDL2/SDL_video.h"
//

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
//

#include "vulkan/vulkan_core.h"
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
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
    SDL_Log("Window Dimensions: %i x %i", screenDims.width, screenDims.height);

     // TODO addd to builder
    // // vulkan features to request from version 1.2
    // VkPhysicalDeviceVulkan12Features features12 = {};
    // features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    // features12.bufferDeviceAddress = true;
    // features12.descriptorIndexing = true;

    //  // vulkan features to request from version 1.3
    // VkPhysicalDeviceVulkan13Features features13 = {};
    // features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    // features13.dynamicRendering = true;
    // features13.synchronization2 = true;

    // TODO TRY to pull some stuff out of render initialize and have init VK systems?
     //TODO should maybe define our own exit_success and failure codes for debugging later
    if (mRenderer.init(appInfo, screenDims) != EXIT_SUCCESS)
    {
      mShouldClose = true;
      return;
    }

    mSceneDataBuffer.allocateBuffer(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    mRenderer.initDefaults(mSceneDataBuffer, &mSceneData);

     // zero out the ticklist for performance tracking
    std::memset(mFrameTimeList, 0, sizeof(mFrameTimeList));

     // create a default texture that gets used whenever assimp cant find a texture
     // Need to load this in first so it can be Texture 0 for default
     // TODO set this as a static variable
    mInput.init(mRenderer.Window());

    // NOTE: must use screen dimension not pixel width and height
    // TODO should make a better distinction and perhaps have it available globally
    mInput.setMouseDeadzone(50, screenDims.width, screenDims.height);

    initPipelines();
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
     // store default textures for when texture file is unfound, etc.
     // TODO handle this with texture atlas
    //mFallbackTexture.loadTexture("plain.png");
    FcLight::loadDefaultTexture("point_light.png");

    // Load the castle object
    FcDescriptorBindInfo bindInfo;
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout dsl = FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);

    FcModel* model = new FcModel{"models/castle.obj", dsl};
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
    model = new FcModel("models/quad.obj", dsl);
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

      //mUbo.pointLights[i] = light->generatePointLight();
    }

    //mUbo.numLights = lightColors.size();


     // add the sun
    FcLight* sun = new FcLight(7.0f, 0.4f, {1.0f, 1.0f, 1.0f});
    sun->setPosition({0.0f, -4.5f, 0.0f});
    // mUbo.pointLights[mUbo.numLights] = sun->generatePointLight();
    // mUbo.numLights += 1;

  } // --- Frolic::loadGameObjects (_) --- (END)


  void Frolic::initPipelines()
  {
    //TODO remove
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GRADIENT PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//    Make sure to initialize the FcPipelineCreateInfo with the number of stages we want
    FcPipelineConfig pipelineConfig{1};
    pipelineConfig.name = "gradient";
    pipelineConfig.shaders[0].filename = "gradient_color.comp.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPushConstantRange pushRange;

    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(ComputePushConstants);

    pipelineConfig.addPushConstants(pushRange);

    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);

    // addBinding() not needed since there's alread a descriptorSetLayout for pipeline
    pipelineConfig.addDescriptorSetLayout(mRenderer.getBackgroundDescriptorLayout());
    // pipelineConfig.disableBlending();
    //pipelineConfig.disableDepthtest();
    // pipelineConfig.enableBlendingAlpha();

    mGradientPipeline.create(pipelineConfig);

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SKY PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    pipelineConfig.name = "Sky Pipeline";
    pipelineConfig.shaders[0].filename = "sky.comp.spv";

    mSkyPipeline.create(pipelineConfig);
    mPipelines.push_back(&mSkyPipeline);
    mPipelines.push_back(&mGradientPipeline);

    mPushConstants[0].data1 = glm::vec4{0.1, 0.2, 0.4, 0.97};
    mPushConstants[1].data1 = glm::vec4(1,0,0,1);
    mPushConstants[1].data2 = glm::vec4(0,1,0,1);
  }


  void Frolic::run()
  {
    // Initialize player controls and position
    FcPlayer player{mInput};
    FcPlayer uvnPlayer{mInput};
    //player.setPosition(glm::vec3(30.f, -00.f, -85.f));
    //player.setPosition(glm::vec3(-5.f, 5.5f, .20f));
    /* player.setPosition(glm::vec3(0.f, 0.f, 2.f)); */
    player.setPosition(glm::vec3(-2.5f, 8.4f, -0.28f));

    uvnPlayer.setPosition(glm::vec3(0.f, 0.f, 2.f));

    player.Camera().setPerspectiveProjection(60.0f, FcLocator::ScreenDims().width
                                             , FcLocator::ScreenDims().height, 512.f, 0.1f);

    /* player.Camera().setOrthographicProjection(-5.f, 5.f, -5.f, 5.f, 30.f, 0.1f); */

    // uvnPlayer.Camera().setPerspectiveProjection(66.0f, FcLocator::ScreenDims().width
    //                                 , FcLocator::ScreenDims().height, 500.f, 0.1f);

    // Initialize simple first person camera
    // FcCamera camera;
    // FcCamera uvnCamera;

    //    mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), 0.01f, 1000.f);
    //    mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), .1f, 30.f);
    //mSceneData.projection = glm::ortho(0.0f, 1350.0f, 900.0f, 0.0f, 0.1f, 1000.0f);
    // mSceneData.projection = glm::orthoRH_ZO(0.0f, 1350.0f, 900.0f, 0.0f, 1111.1f, 0.10f);
    // mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), 500.0f, 0.1f);
    // mSceneData.projection[1][1] *= -1;
    //
    mSceneData.projection = player.Camera().Projection();

    // mSceneData.projection = orthographic(-1.f, 1.f,
    //                                     -1.f, 1.f,  100.f, 0.1f);
    //camera.setViewTarget(glm::vec3{0,0,5}, glm::vec3{0,0,-1});

    int currentBackgroundEffect{0};

    // FIXME
    // SDL_ShowCursor(SDL_DISABLE);

    // default lighting parameters
    // mSceneData.ambientLight = glm::vec4(1.0f, 0.05f, 0.05f, .5f);
    // mSceneData.ambientLight = glm::vec4(1.0f, 0.05f, 0.05f, .3f);
    mSceneData.ambientLight = glm::vec4(0.2f);
//    mSceneData.ambientLight = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
    mSceneData.sunlightColor = glm::vec4(1.f);//, 1.f, 1.f, 1.0);
    //mSceneData.sunlightDirection = glm::vec4(2.f, 10.f, -3.f, 1.f);
    //mSceneData.sunlightDirection = glm::vec4(2.0f, 2.f, 0.f, 1.f);
    mSceneData.sunlightDirection = glm::vec4(0.1f, 1.f, 0.05f, 1.f);

    // load everything we need for the scene
    //loadUIobjects();
    //loadGameObjects();
    // TODO this should be abstracted into engine

    fcLog("Frolic Initialized: Starting main run loop", 0);
    FcTimer mTimer;
    mTimer.start();
    float deltaTime = 0.0f;


    bool ao;

    // TODO separate to make entirely own function
    while (!mShouldClose)
    {
      bool shouldresize = false;
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
              //   shouldresize = true;
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
      mRenderer.stats.frametime = deltaTime;
      mRenderer.stats.fpsAvg = calcFPS(deltaTime);
      // now re-start the time so that the start time is the start of each frame
      mTimer.start();

      // update keyboard and game controller state and use that to mover the player (and thus camera)
      mInput.update();

      // Move the player with the updated input, which will automatically update the camera
      player.move(deltaTime);
      uvnPlayer.moveNew(deltaTime);

      //update(deltaTime);

      // TODO not robust as getview must happen before inverseView
      //camera.setViewTarget(glm::vec3{0,0,4.0}, glm::vec3{0,0,-1});
      mSceneData.eye = glm::vec4(player.Camera().Position(), 1.0f);
      mSceneData.view = player.Camera().getViewMatrix();//View();

      //mSceneData.view = glm::scale(mSceneData.view, glm::vec3(15.0f, 15.0f, 15.0f));
      //mSceneData.view = camera.View();
      // mSceneData.view = glm::translate(glm::vec3{0.f, 0.f, -4.f});

      // TODO pre-calculate  this in camera
      mSceneData.viewProj = mSceneData.projection * mSceneData.view;
      mSceneData.lighSpaceTransform = mRenderer.mShadowMap.LightSpaceMatrix();
//      mSceneDataBuffer.overwriteData(&mSceneData, sizeof(SceneData));

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   START THE NEW FRAME   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // mRenderer.generateShadowMap();


      uint32_t swapchainImgIndex = mRenderer.beginFrame();

      FcPipeline* selected = mPipelines[currentBackgroundEffect];

      mRenderer.attachPipeline(selected);

      // test ImGui UI
      // Left here to add a demo windo that names all the features for (handy for searching)
      // ImGui::ShowDemoWindow();

      // Create Statistics window that spans the frame
      if (ImGui::Begin("Frolic Stats", NULL, ImGuiWindowFlags_NoTitleBar))
      {
        // Stats
        ImGui::Text("Average FPS: %i ", mRenderer.stats.fpsAvg);
        ImGui::SameLine(0.f, 10.f);
        ImGui::Text("| Frame time: %fms", mRenderer.stats.frametime);
        ImGui::SameLine(0.f, 10.f);
        ImGui::Text("| Draw time: %fms", mRenderer.stats.meshDrawTime);
        ImGui::SameLine(0.f, 10.f);
        ImGui::Text("| Update time: %fms", mRenderer.stats.sceneUpdateTime);
        ImGui::SameLine(0.f, 10.f);
        glm::vec4 pos = mSceneData.eye;
        ImGui::Text("| Position: <%.3f,%.3f,%.3f>", pos.x, pos.y, pos.z);
        ImGui::SameLine(0.f, 10.f);
        ImGui::Text("| Triangles rendered: %i", mRenderer.stats.triangleCount);
        ImGui::SameLine(0.f, 10.f);
        ImGui::Text("| Total objects rendered: %i", mRenderer.stats.objectsRendered);
        // int relX, relY;
        // mInput.RelativeMousePosition(relX, relY);
        // ImGui::Text("Mouse X pos: %i", mInput.getMouseX());
        // ImGui::Text("Mouse Y pos: %i", mInput.getMouseY());

        // TODO check the official way to use ImGui via the imgui example source code
        ImGui::End();
      }

      // TODO probably best to enable disable this stuff eventually in a dedicated pipeline shader
      // and then just bind the appropriate pipeline
      // Draw Configuration panel
      // BUG collapsing the control panel kills the program
      if (ImGui::Begin("Scene Data"))
      {
        // TODO update all options with bitfields instead of bools


        if (ImGui::Checkbox("Wire Frame", &mRenderer.drawWireframe))
        {
          // b
        }
        if (ImGui::Checkbox("Color Texture", &mUseColorTexture))
        {
          mRenderer.setColorTextureUse(mUseColorTexture);
        }

        if (ImGui::Checkbox("Rough/Metal Texture", &mUseRoughMetalTexture))
        {
          mRenderer.setRoughMetalUse(mUseRoughMetalTexture);
        }

        if(ImGui::Checkbox("Ambient Occlussion Texture", &mUseOcclussionTexture))
        {
          mRenderer.setAmbientOcclussionUse(mUseOcclussionTexture);
        }

        if (ImGui::Checkbox("Normal Texture", &mUseNormalTexture))
        {
          mRenderer.setNormalMapUse(mUseNormalTexture);
        }
        if (ImGui::Checkbox("Emissive Texture", &mUseEmissiveTexture))
        {
          mRenderer.setEmissiveTextureUse(mUseEmissiveTexture);
        }

        ImGui::Checkbox("Draw Normals", &mRenderer.mDrawNormalVectors);

        ImGui::Checkbox("Box Bounds", &mRenderer.mDrawBoundingBoxes);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Box ID", &mRenderer.mBoundingBoxId);

        ImGui::SetNextItemWidth(60);
        ImGui::SliderInt("Model Rotation Speed", &mRenderer.rotationSpeed, -5, 5);
        ImGui::SetNextItemWidth(60);
        ImGui::SliderFloat("Movement Speed", &player.moveSpeed(), 1, 50, "%.1f");
        ImGui::Checkbox("Cycle", &mCycleExpansion);

        if (mCycleExpansion)
        {
          float time = SDL_GetTicks() / 1000.0f;
          mRenderer.expansionFactor = sin(time) + 1.0f;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::SliderFloat("Expansion Factor", &mRenderer.expansionFactor, -1.f, 2.f);

	//ImGui::SetNextItemWidth(60);
        if(ImGui::SliderFloat4("Sunlight", (float*)&mSceneData.sunlightDirection, -1.f, 1.f))
        {
          glm::vec4 lightPos = mSceneData.sunlightDirection - mSceneData.eye;
          mRenderer.mShadowMap.updateLightSource(lightPos, glm::vec3(0.f, 0.f, 0.f));
        }
        ImGui::Checkbox("Draw Shadow Map", &mDebugShadowMap);
        if(ImGui::SliderFloat("Left", &mRenderer.mShadowMap.Frustum().left, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Right", &mRenderer.mShadowMap.Frustum().right, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Top", &mRenderer.mShadowMap.Frustum().top, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Bottom", &mRenderer.mShadowMap.Frustum().bottom, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Near", &mRenderer.mShadowMap.Frustum().near, -.01f, 10.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Far", &mRenderer.mShadowMap.Frustum().far, 1.f, 100.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }

        if (ImGui::Button("Display View Matrix"))
        {
          ImGui::OpenPopup("MatrixView");
        }
        if (ImGui::BeginPopup("MatrixView"))
        {
          glm::mat4 mat = player.Camera().getViewMatrix();
          glm::mat4 mat2 = uvnPlayer.Camera().getViewMatrix();

          ImGui::Text("Quaternion View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
          ImGui::Text("UVN View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][0], mat2[1][0], mat2[2][0], mat2[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][1], mat2[1][1], mat2[2][1], mat2[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][2], mat2[1][2], mat2[2][2], mat2[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][3], mat2[1][3], mat2[2][3], mat2[3][3]);
          ImGui::EndPopup();
        }
        // TODO getrenderScale should be deleted
        //ImGui::SliderFloat("Render Scale", mRenderer.getRenderScale(), 0.2f, 1.0f);
        //ImGui::Checkbox("Ambient Occlussion Texture", bool *v);
        // ImGui::Text("Selected Effect: %s", selected->Name());
        // ImGui::SliderInt("Efect Index", &currentBackgroundEffect, 0, mPipelines.size() - 1);

        // ImGui::InputFloat4("Data2", (float*)& mPushConstants[currentBackgroundEffect].data2);
        // ImGui::InputFloat4("Data3", (float*)& mPushConstants[currentBackgroundEffect].data3);
        // ImGui::InputFloat4("Data4", (float*)& mPushConstants[currentBackgroundEffect].data4);

        // ??
	//ImGui::EndFrame();
        ImGui::End();

        // make ImGui calculate internal draw structures
        ImGui::Render();
      }

      mSceneDataBuffer.overwriteData(&mSceneData, sizeof(SceneData));
      //mRenderer.drawModels(swapchainImgIndex, mUbo);

      //mRenderer.drawBillboards(camera.Position(), frame, mUbo);

      //mRenderer.drawUI(mUItextList, frame);

      //mRenderer.drawBackground(mPushConstants[currentBackgroundEffect]);

      mRenderer.drawShadowMap(mDebugShadowMap);

      mRenderer.endFrame(swapchainImgIndex);

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

    //      auto rotateLight = glm::rotate( glm::mat4(1.f), i *
    //      glm::two_pi<float>() / lightColors.size()
    //                       , {0.f, -1.f, 0.f});

    //    mUbo.pointLights[i].setPosition(glm::vec3(rotateLight *
    //    glm::vec4(-1.f, -1.f, -1.f, 1.f)));

    // light->setPosition(glm::vec3(rotateLight * glm::vec4(-1.f, -1.f,
    // -1.f, 1.f))); mLights.back().setPosition(glm::vec3(rotateLight *
    // glm::vec4(-1.f, -1.f, -1.f, 1.f)));




    //
    rotateMat = glm::rotate(glm::mat4(1.f), glm::radians(25 * deltaTime),
                            {0.f, -1.f, 0.f});

    auto lights = FcLocator::Lights();
    //
    for (size_t i = 0; i < lights.size() - 1; ++i)
    {
      // light->Transform().translation = glm::vec3(rotateMat *
      // glm::vec4(light->Transform().translation, 1.f));
      glm::vec4 v = lights[i]->getPosition();

      lights[i]->setPosition(rotateMat * v);

      // copy light to ubo
      // mUbo.pointLights[i] = lights[i]->generatePointLight();
    }

     // TODO put timer and FPS stuff in Renderer
     // use the timers elapsed time to calculate the average frames per second
     // PROFILING
    int avgFPS  = calcFPS(deltaTime);
    std::string fpsNum{std::to_string(avgFPS)};

     // TODO editing this text costs about 30-40 fps for some reason, fix
//    VkCommandBuffer cmdBuffer = mRenderer.getCurrentFrame().commandBuffer;

    mUItextList[1].editText(fpsNum, 70, 16, 1.0f);

//    mRenderer.submitCommandBuffer(cmdBuffer);

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

    mInput.kill();

     // free image resources
    mFallbackTexture.destroy();
    //
    mMeshPipeline.destroy();

    // Destroy data buffer with scene data
    mSceneDataBuffer.destroy();

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

    mGradientPipeline.destroy();
    mSkyPipeline.destroy();
     // for (auto& model : mModelList)
     // {
     //   model.destroy();
     // }

    mRenderer.shutDown();
  }

} // namespace fc END
