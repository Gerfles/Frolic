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
  vec4 sunDirection;
  //
} sceneData;

// TODO prefer this style guideline -> change other shaders to this
layout (location = 0) out VS_OUT
{
  vec3 normal;
  vec4 tangent;
} Out;

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
  mat4 model;
  mat4 normalTransform;
  VertexBuffer vertexBuffer;
} surface;

void main()
{
  Vertex v = surface.vertexBuffer.vertices[gl_VertexIndex];
  // Must do the projection transform in the geometry shader since we also need to transform
  // the line end-point that we calculate inside the shader

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROM MESH.VERT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  gl_Position = surface.model * vec4(v.position, 1.0);

  // Out.normal = normalize(mat3(transpose(inverse(surface.model))) * v.normal);
  Out.normal = normalize(mat3(surface.normalTransform) * v.normal);
  Out.tangent = v.tangent;
}
