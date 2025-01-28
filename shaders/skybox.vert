// skybox.vert

#version 450

layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec4 invView;
  vec4 ambientColor;
  vec4 sunDirection;
  vec4 sunColor;
} sceneData;

layout(location = 0) in vec3 vertPos;
layout(location = 0) out vec3 outUVW;

void main()
{
  outUVW = vertPos;
  outUVW.z *= -1.0;

  vec4 position = sceneData.proj * mat4(mat3(sceneData.view)) * vec4(vertPos, 1.0);
  position.z = 0.0;
  gl_Position = position;//(position.x, position.y, 0.0, position.w);
}
