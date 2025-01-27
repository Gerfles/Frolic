#version 450
#extension GL_GOOGLE_include_directive : require

//#include "input_structures.glsl"
layout(set = 0, binding = 0) uniform SceneData
{
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec4 ambientColor;
  vec4 sunDirection;
  vec4 sunColor;
  //
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData
{
  vec4 colorFactors;
  vec4 MetalRoughFactors;
  //
} materialData;


layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;


layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosWorld;


layout (location = 0) out vec4 outFragColor;

void main()
{
  // new

  vec3 color = inColor * texture(colorTex, inUV).xyz;
  vec3 diffuseLight = color * sceneData.ambientColor.xyz * sceneData.ambientColor.w;
  // vec3 diffuseLight = sceneData.ambientColor.xyz * sceneData.ambientColor.w;

  // specular light calc

  // get the camera position in world space
  // TODO should pass this in so it's not so computationally expensive
  vec3 cameraPosWorld = inverse(sceneData.view)[3].xyz;
  vec3 viewDirection = normalize(cameraPosWorld - inPosWorld);

  // No attenuation for sunlight but here for example
  float attenuation = 1.0f;// / dot(sceneData.sunDirection, sceneData.sunDirection);
  vec3 intensity = sceneData.sunColor.xyz * sceneData.sunColor.w * attenuation;

  vec3 directionToLight = normalize(sceneData.sunDirection.xyz);
  vec3 surfaceNormal = normalize(inNormal);
  float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.0f);

  diffuseLight += intensity * cosAngIncidence;

  vec3 halfAngle = normalize(directionToLight + viewDirection);
  float blinnTerm = dot(surfaceNormal, halfAngle);
  blinnTerm = clamp(blinnTerm, 0.f, 1.f);
  blinnTerm = pow(blinnTerm, 64.0f);
  vec3 specularLight = intensity * blinnTerm;

  outFragColor = vec4(color * (diffuseLight + specularLight), 1.0);
}
