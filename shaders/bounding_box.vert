// bounding_box.vert

#version 450

const vec3 CUBE_VERTICES[36] =
  vec3[]
  (
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-  -Z FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-  -X FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   +X FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   +Z FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f, -1.0f,  1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   +Y FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   -Y FACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vec3(-1.0f,  -1.0f, -1.0f),
    vec3(1.0f,  -1.0f, -1.0f),
    vec3(1.0f,  -1.0f,  1.0f),
    vec3(1.0f,  -1.0f,  1.0f),
    vec3(-1.0f,  -1.0f,  1.0f),
    vec3(-1.0f,  -1.0f, -1.0f)
   );

// TODO don't need much of this data but provides for easier descriptor binding for now.
// Eventually pair down some of the data for all the "helper" methods...
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


layout(push_constant) uniform constants
{
  mat4 modelMatrix;
  vec4 origin;
  vec4 extents;
} box;

void main()
{
  vec4 cornerVertex = vec4(box.origin.xyz + (CUBE_VERTICES[gl_VertexIndex] * box.extents.xyz), 1.0);

  gl_Position =  sceneData.viewProj * box.modelMatrix * cornerVertex;
}
