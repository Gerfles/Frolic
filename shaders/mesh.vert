#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

//#include "input_structures.glsl"

// TODO think about only including this in vertex shader and passing needed to fragment
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
  //
} sceneData;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outTangent;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outPosWorld;

struct Vertex
{
   vec3 position;
   float uv_x;
   vec3 normal;
   float uv_y;
   vec4 tangent;
};


layout (buffer_reference, std430) readonly buffer VertexBuffer
{
  Vertex vertices[];
};

// push constants block
layout(push_constant) uniform constants
{
  mat4 renderMatrix;
  mat4 normalTransform;
  VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
  // try 1000.0f instead of 10000.0f in persp...
  // UniformData.light = vec4s{2.0f, 2.0f, 0.0f, 1.0f}
  // alignas(16) MaterialData
  // MaterialData MaterialConstant.model

  Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

  vec4 positionWorld = PushConstants.renderMatrix * vec4(v.position, 1.0f);
  outPosWorld = positionWorld.xyz;

  //gl_Position = sceneData.proj * sceneData.view * PushConstants.renderMatrix * vec4(v.position, 1.0f);
  gl_Position = sceneData.viewProj * positionWorld;
  //gl_Position = sceneData.proj * sceneData.view * positionWorld;

  // outNormal = mat3(sceneData.proj * sceneData.view * PushConstants.normalTransform) * v.normal;
   outNormal = mat3(PushConstants.normalTransform) * v.normal;
  // outNormal = mat3(inverse(transpose(PushConstants.renderMatrix))) * v.normal;

  outUV.x = v.uv_x;
  outUV.y = v.uv_y;

  outTangent = v.tangent;

}
