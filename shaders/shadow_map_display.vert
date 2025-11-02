// shadow_map_display.vert

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

// TODO set push constant max to be 128 for most implementations (guaranteed amount)
layout (push_constant) uniform constants
{
  mat4 MVP;
  VertexBuffer vertexBuffer;
} push;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float zNear;
layout (location = 2) out float zFar;

void main()
{
  zNear = push.MVP[0][0];
  zFar = push.MVP[1][1];

  // surprisingly works!... ?? Find out how and if works with 3 vertices instead of 6
  // vertIndex = 0 -> outUV = <0, 0>   | gl_Position = <-1, -1, 0, 1>
  // vertIndex = 1 -> outUV = <2, 0>   | gl_Position = <3, -1, 0, 1>
  // vertIndex = 2 -> outUV = <0, 2>   | gl_Position = <-1, 3, 0, 1>
  outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

  gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
