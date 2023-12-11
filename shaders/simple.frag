// simple.frag

#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inPosWorld;
layout (location = 2) in vec3 inNormalWorld; 
 
layout (location = 0) out vec4 outColor;

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
  vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  vec3 specularLight = vec3(0.0);
  vec3 surfaceNormal = normalize(inNormalWorld);

  // get the camera position in world space
  vec3 cameraPosWorld = ubo.invView[3].xyz;
  vec3 viewDirection = normalize(cameraPosWorld - inPosWorld);
  
  for (int i = 0; i < ubo.numLights; i++)
  {
    PointLight light = ubo.pointLights[i];
    vec3 directionToLight = light.position.xyz - inPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
    directionToLight = normalize(directionToLight);
    
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;

    diffuseLight += intensity * cosAngIncidence;

     // specular lighting (Blinn-Phong)
    vec3 halfAngle = normalize(directionToLight + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 128.0); // higher values -> sharper highlight
     // below was a mistake but somehow worked as a good shinny approx
     //specularLight += light.color.xyz * attenuation * blinnTerm;
    specularLight += intensity * blinnTerm;
  }

  outColor = vec4(diffuseLight * inColor + specularLight  * inColor, 1.0); 
}
