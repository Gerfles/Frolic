// UI VERTEX SHADER

#version 450

// TODO remove the unneeded bindings from FcPipeline for billboard
// layout(location = 0) in vec2 position;
// layout(location = 1) in vec2 color;
// layout(location = 2) in vec2 tex;


layout(set = 0, binding = 0) uniform UboViewProjection
{
  mat4 projection;
  mat4 view;
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

const vec2 SCREEN_VERTICES[6] =
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
  // first pick the appropriate vertex from the box and scale it by the box width and height
  vec2 boxVertex = SCREEN_VERTICES[gl_VertexIndex] * vec2(push.width, push.height);

  // next translate the box vertex to the passed in position and make into a vec4 w/ z=0
  gl_Position = vec4(boxVertex + push.position.xy, 0.0, 1.0);

  // vec3 cameraRightWorld = {ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]};
  // vec3 cameraUpWorld = {ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]};
  // vec3 positionWorld = push.position.xyz
  //                      + push.width * cornerVertex.x * cameraRightWorld
  //                      + push.height * cornerVertex.y * cameraUpWorld;
   
  // gl_Position = ubo.projection * ubo.view * vec4(positionWorld, 1.0);

  // IMPROVED Method

  // ndc coordinates


//  vec4 boxCorner = vec4(cornerVertex + push.position.xy, 0.0, 1.0) * vec4(push.width, push.height, 0.0, 1.0);
  // cornerVertex = cornerVertex * push.position.xy;
  // vec4 boxCorner = vec4(cornerVertex.x + push.width, cornerVertex.y + push.height, 0.0, 1.0);


  
  // //vec4 boxCorner = vec4(cornerVertex.x * push.width, cornerVertex.y * push.position.y, 0.0, 1.0);
  // //vec4 boxCorner = vec4(cornerVertex / 2, 0.0, 1.0);

  
  //  // first transform billboard position to camera space
  //  //  vec4 billboardCenter = ubo.view * vec4(push.position.xzy, 1.0);
  
  // // now apply the offsets in camera space
  // // vec4 billboardCorner = billboardCenter + push.width * vec4(cornerVertex.x, 0.0, 0.0, 0.0)
  // //                        + push.height * vec4(0.0, cornerVertex.y, 0.0, 0.0);

  // // finally apply the projection transform to get us to clip space
  // //gl_Position = ubo.projection * billboardCorner;
  // gl_Position = boxCorner;

  texCoords = UV_COORDS[gl_VertexIndex];

  fragColor = push.color;
  
}
