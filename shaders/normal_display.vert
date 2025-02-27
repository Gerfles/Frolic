// normal_display.vert

#version 450

#extension GL_EXT_buffer_reference : require

layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  mat4 lightSpaceTransform;
  vec4 ambientColor;
  vec4 sunDirection;
  vec4 sunColor;
  //
} sceneData;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outTangent;

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
  Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
  // Must do the projection transform in the geometry shader since we also need to transform
  // the line end-point that we calculate inside the shader
  gl_Position = sceneData.view * PushConstants.renderMatrix * vec4(v.position, 1.0f);

  outNormal = normalize(mat3(sceneData.view * PushConstants.normalTransform) * v.normal);
  //outNormal = normalize(mat3(PushConstants.normalTransform) * v.normal);
  outTangent = v.tangent;
}
