// shadow_map.vert

#version 450
#extension GL_EXT_buffer_reference : require

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

layout (push_constant) uniform constants
{
  mat4 lightSpaceMatrix;
  mat4 model;
  VertexBuffer vertexBuffer;
} push;

//layout (location = 0) out vec4 outPosition;

out gl_PerVertex
{
  vec4 gl_Position;
};

void main()
{
  Vertex vert = push.vertexBuffer.vertices[gl_VertexIndex];
//  mat4 id = mat4(1.0);

  vec4 pos = push.lightSpaceMatrix * push.model * vec4(vert.position, 1.0);

  gl_Position =  vec4(pos.x, pos.y, pos.z, pos.w);
  //outPosition = push.lightSpaceMatrix * pos;
}
