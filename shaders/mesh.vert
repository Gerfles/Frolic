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

layout(std140, set = 1, binding = 0) uniform MaterialConstants
{
  vec4 colorFactors;
  vec4 MetalRoughFactors;
  vec4 emissiveFactors; // w = emissive strength
  //
  float occlusionFactor;
  float iorF0;
  uint flags;
} materialData;

// TODO check if we can remove these since not accessed in this stage
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalMap;
layout(set = 1, binding = 4) uniform sampler2D occlusionMap;
layout(set = 1, binding = 5) uniform sampler2D emissiveMap;

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

  gl_Position = sceneData.viewProj * positionWorld;

  //outNormal = (PushConstants.renderMatrix * vec4(v.normal, 0.f)).xyz;
  // TODO might be slightly faster but might be incorrect
  //outNormal = mat3(PushConstants.renderMatrix) * v.normal;

  // TODO pass in inverses for uniforms but I think the below is the proper transform
  // ?? one that deals with non-uniform scaling
  outNormal = mat3(PushConstants.normalTransform) * v.normal;

  outUV.x = v.uv_x;
  outUV.y = v.uv_y;

  outTangent = v.tangent;
}
