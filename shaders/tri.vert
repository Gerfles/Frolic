// TRI VERTEX SHADER - SWITCH TO VERTEX SHADER WITH s-p, a
// use GLSL version 4.5
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 inTexCoords;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 outTexCoords;


struct PointLight
{
   vec4 position; // ignore w
   vec4 color; // w is intensity
};

// a uniform comes into the shader pipeline once and is UNIFORM for all vertices--this is unlike the
// in and out variables uniforms are actually short for Uniform Buffer Object. Emphasis on the
// OBJECT because these must be a struct-like definition and cannot be single values
// Note: there is an implicit set variable here that is zero if not defined, though I'm adding it in to
// increase readability
layout(set = 0, binding = 0) uniform GlobalUbo
{
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;


layout(push_constant) uniform Push
{
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;


void main()  
{
   // calculate vertex location
  vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
  gl_Position = ubo.projection * ubo.view * positionWorld;
  
   // - VERTEX NORMALS -
  
   // Method 1. This is only correct i f all scaling is uniform (sX == sY == sZ)!
   // but this method is by far the fastest and should use if we don't plan on utilizing non-uniform scales
   //vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal);

   // Method 2. Correct even with non-uniform scaling but computationally expensive
   // mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
   // fragNormalWorld = normalize(normalMatrix * normal);
   // fragPosWorld = positionWorld.xyz;
  
   // Method 3. Pass in pre-computed normal matrix to shaders-adds complexity, but better than calculating normal matrix each vertex
   //TODO check normalize later
  fragNormalWorld = mat3(push.normalMatrix) * normal;
  fragPosWorld = positionWorld.xyz;
  
  outColor = inColor;
  outTexCoords = inTexCoords;

}

                 
