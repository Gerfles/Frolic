#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outUV;

struct Vertex
{
   vec3 position;
   float uv_x;
   vec3 normal;
   float uv_y;
   vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
  Vertex vertices[];
};

// push constants block
layout (push_constant) uniform constants
{
  mat4 renderMartix;
  VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
   // load vertex data from device address
  Vertex vert = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

   // output data
  gl_Position = PushConstants.renderMartix * vec4(vert.position, 1.0f);
  outColor = vert.color.xyz;
  outUV.x = vert.uv_x;
  outUV.y = vert.uv_y;
}