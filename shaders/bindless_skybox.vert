//>_ bindless_skybox.vert _<//
#version 450

const vec3 CUBE_VERTICES[36] =
  vec3[]
  ( // positions
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    //
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    //
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    //
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f, -1.0f,  1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    //
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(1.0f,  1.0f, -1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(1.0f,  1.0f,  1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    //
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    vec3(1.0f, -1.0f,  1.0)
   );


layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  mat4 lightSpaceTransform;
  vec4 sunDirection;
} sceneData;

layout(location = 0) out vec3 outUVW;

void main()
{
  // Must remove the translation part of view matrix and then (fill with zeros) then
  // multiple by projection so that the cube never approaches us
  outUVW = CUBE_VERTICES[gl_VertexIndex];

  vec4 position = sceneData.proj * mat4(mat3(sceneData.view))
                  * vec4(outUVW, 1.0);

  // set z to min depth so we can write to depthbuffer and draw skycube last, that
  // way we are not drawing a fragment for every pixel in the screen.
  position.z = 0.0;
  gl_Position = position;
}
