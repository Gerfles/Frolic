#version 450

//#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

//#include "input_structures.glsl"

// TODO think about only including this in vertex shader and passing needed to fragment
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

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outTangent;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outPosWorld;
layout (location = 4) out vec4 outPosLightSpace;

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

// TODO too much to pass via PCs since normal limit is ~128 bytes
// push constants block
layout(push_constant) uniform PushConstants
{
  mat4 renderMatrix;
  mat4 normalTransform;
  VertexBuffer vertexBuffer;
} model;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main()
{
  // try 1000.0f instead of 10000.0f in persp...
  // UniformData.light = vec4s{2.0f, 2.0f, 0.0f, 1.0f}
  // alignas(16) MaterialData
  // MaterialData MaterialConstant.model

  Vertex v = model.vertexBuffer.vertices[gl_VertexIndex];

  vec4 positionWorld = model.renderMatrix * vec4(v.position, 1.0);

  // TODO find out which version is correct
  outPosWorld = positionWorld.xyz;
  // outPosWorld = (sceneData.view * vec4(positionWorld.xyz, 1.0)).xyz;
  gl_Position = sceneData.viewProj * positionWorld;
  // gl_Position = sceneData.viewProj * model.renderMatrix * vec4(v.position, 1.0f);

  //gl_Position = vec4(v.position, 1.0);

  //gl_Position = sceneData.proj * sceneData.view * positionWorld;

  // TODO verify equivalence
//  outPosLightSpace = sceneData.lightSpaceTransform * positionWorld;
  // outPosLightSpace = (biasMat * sceneData.lightSpaceTransform * model.renderMatrix) * vec4(v.position, 1.0);
  outPosLightSpace = (sceneData.lightSpaceTransform * model.renderMatrix) * vec4(v.position, 1.0);
  // outNormal = mat3(sceneData.proj * sceneData.view * model.normalTransform) * v.normal;
  outNormal = mat3(model.normalTransform) * v.normal;
  // outNormal = mat3(inverse(transpose(model.renderMatrix))) * v.normal;

  outUV.x = v.uv_x;
  outUV.y = v.uv_y;

  outTangent = v.tangent;
}
