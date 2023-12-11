// simple.vert

#version 450 // use GLSL 4.5

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 uv;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outPosWorld;
layout(location = 2) out vec3 outNormalWorld;

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
  vec4 ambientLightColor; // w  is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push
{
  mat4 modelMatrix; // projection * view * model
  //mat4 modelMatrixOnly; // needed if using method 1 or 2
  mat4 normalMatrix;
} push;


void main()
{ 
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
  gl_Position =  ubo.projection * ubo.view * positionWorld;

   // Method 1. This is only correct i f all scaling is uniform (sX == sY == sZ)!
   //vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal);

   // Method 2. Correct even with non-uniform scaling but computationally expensive
   // mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
   // vec3 normalWorldSpace = normalize(normalMatrix * normal);

   // Method 3. Pass in pre-computed normal matrix to shaders-adds complexity
//TODO check normalize later
  outNormalWorld  = mat3(push.normalMatrix) * normal;
  outPosWorld = positionWorld.xyz;

  // ?? since we have a texture sampler, is there a reason we need to pass through color?
  outColor = inColor;
}
