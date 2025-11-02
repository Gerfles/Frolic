//>_ bindless_skybox.frag _<//

#version 450

layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  mat4 lightSpaceTransform;
  vec4 sunDirection;
  //
} sceneData;

layout(set = 1, binding = 0) uniform samplerCube skyboxImg;

layout(location = 0) in vec3 inUVW;
layout(location = 0) out vec4 fragColor;


void main()
{
  fragColor = texture(skyboxImg, inUVW);
}
