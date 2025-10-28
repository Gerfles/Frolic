//>_ bindless_billboard.vert _<//

#version 450

layout(set = 0, binding = 0) uniform BillboardUbo
{
  mat4 view;
  mat4 projection;
} ubo;


// billboard specific parameters
  layout(push_constant) uniform Push
  {
    vec3 position;
    float width;
    float height;
    uint texIndex;
  } push;

layout (location = 0) out vec2 texCoords;

const vec2 CORNER_VERTICES[6] =
  vec2[]
  (
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
   );

const vec2 UV_COORDS[6] =
  vec2[]
  (
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
   );



void main()
{
//vec2 fragOffset = CORNER_VERTICES[gl_VertexIndex];

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   METHOD 1   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  // vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
  // vec3 positionWorld = push.position.xyz
  //                      + push.width * fragOffset.x * cameraRightWorld
  //                      + push.height * fragOffset.y * cameraUpWorld;
// gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);


  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   METHOD 2   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
   // first transform light position to camera space, then apply offset in camera space
  // vec4 lightInCameraSpace = ubo.view * vec4(push.position.xyz, 1.0);
  // vec4 positionInCameraSpace = lightInCameraSpace + push.width * vec4(fragOffset, 0.0, 0.0);


  // gl_Position = ubo.projection * ubo.view * vec4(positionInCameraSpace);

  vec2 cornerVertex = CORNER_VERTICES[gl_VertexIndex];

  // // vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  // // vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
  // // vec3 positionWorld = push.position.xyz
  // //                      + push.width * cornerVertex.x * cameraRightWorld
  // //                      + push.height * cornerVertex.y * cameraUpWorld;
  // //
  // //  gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   IMPROVED METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

   // first transform billboard position to camera space
  vec4 billboardCenter = ubo.view * vec4(push.position, 1.0f);

  // now apply the offsets in camera space
  vec4 billboardCorner = billboardCenter + push.width * vec4(cornerVertex.x, 0.0, 0.0, 0.0)
                         + push.height * vec4(0.0, cornerVertex.y, 0.0, 0.0);

  // finally apply the projection transform to get us to clip space
  gl_Position = ubo.projection * billboardCorner;

  texCoords = UV_COORDS[gl_VertexIndex];

  // fragColor = push.color;//vec4(push.color.xyz, );
  // fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
