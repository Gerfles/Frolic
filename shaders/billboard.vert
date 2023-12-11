// BILLBOARD VERTEX SHADER

#version 450

// TODO remove the unneeded bindings from FcPipeline for billboard
// layout(location = 0) in vec2 position;
// layout(location = 1) in vec2 color;
// layout(location = 2) in vec2 tex;

struct PointLight
{
   vec4 position; // ignore w
   vec4 color; // w is intensity
};


layout(set = 0, binding = 0) uniform GlobalUbo
{
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor;
  PointLight pointLights[10];
  int numLights;
} ubo;


// billboard specific parameters
  layout(push_constant) uniform Push
  {
    vec4 position;
    vec4 color;
    float width;
    float height;
  } push;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec2 texCoords;

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
   // method 1
  // vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  // vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
  // vec3 positionWorld = push.position.xyz
  //                      + push.width * fragOffset.x * cameraRightWorld
  //                      + push.height * fragOffset.y * cameraUpWorld;
// gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);


// method 2
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

  // IMPROVED Method

   // first transform billboard position to camera space
  vec4 billboardCenter = ubo.view * push.position;
  
  // now apply the offsets in camera space
  vec4 billboardCorner = billboardCenter + push.width * vec4(cornerVertex.x, 0.0, 0.0, 0.0)
                         + push.height * vec4(0.0, cornerVertex.y, 0.0, 0.0);

  // finally apply the projection transform to get us to clip space
  gl_Position = ubo.projection * billboardCorner;

  texCoords = UV_COORDS[gl_VertexIndex];

  fragColor = push.color;//vec4(push.color.xyz, );
  // fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
