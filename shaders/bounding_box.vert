// bounding_box.vert

#version 450

// // TODO see if we could just use the screen corners instead
// const vec3 SCREEN_CORNERS[8] =
//   vec3[]
//   (
//     vec3(1.0, 1.0, 1.0),
//     vec3(1.0, 1.0, -1.0),
//     vec3(1.0, -1.0, 1.0),
//     vec3(1.0, -1.0, -1.0),
//     vec3(-1.0, 1.0, 1.0),
//     vec3(-1.0, 1.0, -1.0),
//     vec3(-1.0, -1.0, 1.0),
//     vec3(-1.0, -1.0, -1.0)
//    );


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
  mat4 lightSpaceTransform;
  vec4 sunDirection;
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

  cornerVertex = cornerVertex / cornerVertex.w;

  gl_Position =  sceneData.viewProj * box.modelMatrix * cornerVertex;
}
