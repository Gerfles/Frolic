#version 450

#extension GL_EXT_buffer_reference : require

// TODO if we don't end up using padding, probably best to send in vec4 position
// instead and then a vec2 pad with uv_x & uv_y
struct Vertex
{
   vec3 position;
   float uv_x;
   vec3 normal;
   float uv_y;
   vec4 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
  Vertex vertices[];
};

layout(push_constant, std430) uniform PushConstants
{
  VertexBuffer vertexBuffer;
  VertexBuffer padding;
} terrain;

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec3 outNormal;

void main()
{
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CPU TERRAIN   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //  Vertex vertex = terrain.vertexBuffer.vertices[gl_VertexIndex];

  // outHeight = vertex.position.y;
  // vec4 positionWorld = terrain.model * vertex.position;

  // gl_Position = sceneData.viewProj * positionWorld;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GPU TERRAIN   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  Vertex vertex = terrain.vertexBuffer.vertices[gl_VertexIndex];

  // convert xyz vertex to xyzw homogeneous coordinate
  gl_Position = vec4(vertex.position, 1.0);

  outTexCoord = vec2(vertex.uv_x, vertex.uv_y);
  outNormal = vertex.normal;
  // // convert xyz vertex to xyzw homogeneous coordinate
  //gl_Position = vec4(1.0, 1.0, 1.0, 1.0);

  // outTexCoord = vec2(0.0, 0.0);

}
