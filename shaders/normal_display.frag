// normal_display.frag
#version 450

// layout(std140, set = 0, binding = 0) uniform SceneData
// {
//   vec4 eye;
//   mat4 view;
//   mat4 proj;
//   mat4 viewProj;
//   vec4 invView;
//   vec4 ambientColor;
//   vec4 sunDirection;
//   vec4 sunColor;
//   //
// } sceneData;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inTangent;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec4 FragColor;

void main()
{
  FragColor = vec4(inColor, 1.0);
}
