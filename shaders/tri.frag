// FRAGMENT SHADER - SWITCH TO VERTEX SHADER WITH s-p, a
// use GLSL version 4.5
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 texCoords;

layout(location = 0) out vec4 outColor; // final output color (out and in are different locations)

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
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

// // TODO delete and remove from push specs
// layout(push_constant) uniform Push
// {
//   mat4 modelMatrix;
//   mat4 normalMatrix;
// } push;



void main()
{
  vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  vec3 specularLight = vec3(0.0);
   // TODO make sure we don't normalize twice if we don't need to 
  vec3 surfaceNormal = normalize(fragNormalWorld);

   // get the camera position in world space
  vec3 cameraPosWorld = ubo.invView[3].xyz;
  vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

  for (int i = 0; i < ubo.numLights; i++)
  {
    PointLight light = ubo.pointLights[i];
    
     // first make the direction of light the vector distance from the fragment to the light
    vec3 directionToLight = light.position.xyz - fragPosWorld;
     // now attenuate that light so it falls off proportional to the distance squared to the fragment
    float attenuation = 1.0 / dot(directionToLight, directionToLight);
     // finally make direction to light an actualy direction vector by normalizing it
    directionToLight = normalize(directionToLight);
     //
    vec3 intensity = light.color.xyz * light.color.w * attenuation;
     // clamp cosine of angle of incidence to zero;
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
     // add the diffuse contribution from the current light to the fragment 
    diffuseLight += intensity * cosAngIncidence;
     // add the specular lighting (Blinn-Phong)
    vec3 halfAngle = normalize(directionToLight + viewDirection);
//    float blinnTerm = clamp(dot(surfaceNormal, halfAngle), 0, 1);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
     // higher values -> sharper highlight
    blinnTerm = pow(blinnTerm, 128.0);
    //
    specularLight += intensity * blinnTerm;
  }
  
   //  //vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
   // vec3 lightColor = vec3(1.0, 1.0, 1.0) * attenuation;

   //  //vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
   // vec3 ambientLight = vec3(1.0, 1.0, 1.0) * 0.2;

   // vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);
 
   //vec3 lightIntensity = vec4((diffuseLight + ambientLight) * fragColor, 1.0);

  
  
//  outColor = texture(textureSampler, texCoords) * vec4(diffuseLight + ambientLight, 1.0);
  outColor = texture(textureSampler, texCoords) * vec4(diffuseLight + specularLight, 1.0);

  // if (ubo.numLights == 5)
  // {
  //   outColor = texture(textureSampler, texCoords) * vec4(1.0, 1.0, 1.0, 1.0);
  // }

}
