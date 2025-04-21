#version 450
#extension GL_GOOGLE_include_directive : require

uint HasColorTexture = 1 << 0;
uint HasNormalTexture = 1 << 1;
uint HasRoughMetalTexture = 1 << 2;
uint HasOcclusionTexture = 1 << 3;
uint HasEmissiveTexture = 1 << 4;
uint HasVertexTangentAttribute = 1 << 5;
uint HasVertexTextureCoordinates = 1 << 6;

//uint MAX_ENUM_FLAG = 0xFFFFFFFF;

//#include "input_structures.glsl"
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

//TODO DELETE?? may need for reflection as well as chrome and glass effects
layout(set = 1, binding = 0) uniform samplerCube skybox;
//layout(input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput shadowMap;
layout(set = 2, binding = 0) uniform sampler2D shadowMap;

layout(std140, set = 3, binding = 0) uniform MaterialConstants
{
  vec4 colorFactors;
  vec4 MetalRoughFactors;
  vec4 emissiveFactors; // w = emmisive strength
  //
  float occlusionFactor;
  float iorF0;
  uint flags;
} materialData;


layout(set = 3, binding = 1) uniform sampler2D colorTex;
layout(set = 3, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 3, binding = 3) uniform sampler2D normalMap;
layout(set = 3, binding = 4) uniform sampler2D occlusionMap;
layout(set = 3, binding = 5) uniform sampler2D emissiveMap;


// TODO
// consider normalizing and cross producting in vertex shader... not sure why not done there
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inTangent;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosWorld; // vertPosition
layout (location = 4) in vec4 inPosLightSpace;

const float PI = 3.14159265358;


layout (location = 0) out vec4 outFragColor;

float shadowCalculation(vec4 posLightSpace, vec3 normal, vec3 lightDirection);

// Normal Distribution Functions
float TrowbridgeReitzNormalDistribution(float NdotH, float roughnessSquared)
{
  float denom = NdotH * NdotH * (roughnessSquared - 1.0f) + 1.0f;
  //return (roughnessSquared * step(0.0f, NdotH)) / (PI * dDenom * dDenom);
  return roughnessSquared / (PI * denom * denom);
}


// Geometric Shadow functions
float SchlickGeometricShadowingFunction(float NdotL, float NdotV, float roughnessSquared)
{
  float SmithL = (NdotL) / (NdotL * (1.0f - roughnessSquared) + roughnessSquared);
  float SmithV = (NdotV) / (NdotV * (1.0f - roughnessSquared) + roughnessSquared);

  return (SmithL * SmithV);
}

// Fresnel Functions
float MixFunction(float i, float j, float x)
{
  return j * x + i * (1.0f - x);
}

float SchlickFresnel(float i)
{
  float x = clamp(1.0f - i, 0.0f, 1.0f);
  float x2 = x * x;
  return x2 * x2 * x;
}

// normal incidence reflection calculation
float F0(float NdotL, float NdotV, float LdotH, float roughness)
{
  float fresnelLight = SchlickFresnel(NdotL);
  float fresnelView = SchlickFresnel(NdotV);
  float fresnelDiffuse90 = 0.5f + 2.0f * LdotH * LdotH * roughness;

  return MixFunction(1, fresnelDiffuse90, fresnelLight) * MixFunction(1.0f, fresnelDiffuse90, fresnelView);
}

vec3 SchlickFresnelFunction(vec3 specularColor, float LdotH)
{
  return specularColor + (1.0f - specularColor) * SchlickFresnel(LdotH);
}

float SchlickIORFresnelFunction(float ior, float LdotH)
{
  float f0 = pow(ior - 1.0f, 2) / pow(ior + 1.0f, 2);
  return f0 + (1.0f - f0) * SchlickFresnel(LdotH);
}


vec3 decode_srgb(vec3 color)
{
  vec3 result;
  //RED
  if (color.r < 0.04045) { result.r = color.r / 12.92; }
  else { result.r = pow((color.r + 0.055) / 1.055, 2.4); }
  //GREEN
  if (color.g <= 0.04045) { result.g = color.g / 12.92; }
  else { result.g = pow((color.g + 0.055) / 1.055, 2.4); }
  //BLUE
  if (color.b <= 0.04045) { result.b = color.b / 12.92; }
  else { result.b = pow((color.b + 0.055) / 1.055, 2.4); }

  return clamp(result, 0.0, 1.0);
}

vec3 encode_srgb(vec3 color)
{
  vec3 result;
  // Red
  if (color.r <= 0.0031308) { result.r = color.r * 12.92; }
  else { result.r = 1.055 * pow(color.r, 1.0 / 2.4) - 0.055; }
  // Green
  if (color.g <= 0.0031308) { result.g = color.g * 12.92; }
  else { result.g = 1.055 * pow(color.g, 1.0 / 2.4) - 0.055; }
  // Blue
  if (color.b <= 0.0031308) { result.b = color.b * 12.92; }
  else { result.b = 1.055 * pow(color.b, 1.0 / 2.4) - 0.055; }

  return clamp(result, 0.0, 1.0);
}


float heaviside(float v)
{
  if (v > 0.0) return 1.0;
  // else
  return 0.0;
}


void main()
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   COLOR   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // calculate color first since it may be discarded immediately if alpha < 0.5
  vec4 baseColor = materialData.colorFactors;
  vec4 albedo = baseColor;

  if ((materialData.flags & HasColorTexture) == HasColorTexture)
  {
    albedo = texture(colorTex, inUV);
    baseColor.rgb *= decode_srgb(albedo.rgb);
    baseColor.a *= albedo.a;
  }

  if (albedo.a < 0.5)
  {
    discard;
  }


  vec3 normalDirection = normalize(inNormal);
  vec3 preNormal = normalDirection;

  // -*-*-*-*-*-*-*-*-*-*-*-*-   USING PASSED IN TANGENT   -*-*-*-*-*-*-*-*-*-*-*-*- //
  mat3 TBN = mat3(1.0f);

  if ((materialData.flags & HasVertexTangentAttribute) == HasVertexTangentAttribute)
  {
    vec3 tangent = normalize(inTangent.xyz);
    vec3 biTangent = cross(normalDirection, tangent) * inTangent.w;

    TBN = mat3(tangent, biTangent, normalDirection);
  }
  else // -*-*-*-*-*-*-   COMPUTING THE TANGENT W/O VERTEX ATTRIBUTE   -*-*-*-*-*-*- //
  {
    //  https://community.khronos.org/t/computing-the-tangent-space-in-the-fragment-shader/52861
    vec3 Q1 = dFdx(inPosWorld);
    vec3 Q2 = dFdy(inPosWorld);
    vec2 st1 = dFdx(inUV);
    vec2 st2 = dFdy(inUV);

    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = normalize(-Q1 * st2.s + Q2 * st1.s);

    // the transpose of texture-to-eye space matrix
    TBN = mat3(T, B, normalDirection);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NORMAL MAP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  if ((materialData.flags & HasNormalTexture) == HasNormalTexture)
  {
    // Normal textures are encoded to [0, 1] but need to be mapped to [-1, 1]
    normalDirection = normalize(texture(normalMap, inUV).rgb * 2.0 - 1.0);
    //normalDirection = decode_srgb(normalDirection);

    normalDirection = normalize(TBN * normalDirection);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   WORLD CALCULATIONS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // vec3 cameraPosWorld = normalize(inverse(sceneData.view)[3].xyz);
  // mat4 inverseView = inverse(sceneData.view);
  // vec3 cameraPosWorld = inverseView[3].xyz;
  vec3 viewDirection = normalize(sceneData.eye.xyz - inPosWorld);
  // TODO this might be the case for sun(ambient) light but not for point lights
  //vec3 lightDirection = normalize(-sceneData.sunDirection.xyz);// - inPosWorld);
  vec3 lightDirection = normalize(sceneData.sunDirection.xyz);// - inPosWorld);
//  vec3 lightDirection = normalize(-(2,10,0) + inPosWorld);// - inPosWorld);
//  vec3 lightDirection = normalize(sceneData.sunDirection.xyz - inPosWorld);
  //vec3 lightDirection = normalize(inPosWorld - sceneData.sunDirection.xyz);

  //vec3 lightDirection = normalize(mix(sceneData.sun))
  vec3 halfDirection = normalize(viewDirection + lightDirection);

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CHROME EFFECT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // // Get reflected view ray from the surface to calculate cube map reflection
  // //TODO do in one call
  // vec3 incident = -viewDirection;
  // vec3 reflection = reflect(incident, normalDirection);
  // vec4 reflectedColor = vec4(texture(skybox, reflection).rgb, 1.0);
  // outFragColor = reflectedColor;
  // return;

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GLASS/WATER/ETC EFFECT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // float iorRatio = 1.00 / 1.309;
  // vec3 incident = -viewDirection;
  // vec3 refraction = refract(incident, normalDirection, iorRatio);
  // outFragColor = vec4(texture(skybox, refraction).rgb, 1.0);
  // return;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CORRECTION FACTORS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float shiftAmount = dot(inNormal, viewDirection);
  normalDirection = shiftAmount < 0.0f
                    ? normalDirection + viewDirection * (shiftAmount + 1e-5f) : normalDirection;

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PBR CONSTANTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float metalness = materialData.MetalRoughFactors.x;
  float roughness = materialData.MetalRoughFactors.y;

  if ((materialData.flags & HasRoughMetalTexture) == HasRoughMetalTexture)
  {
    vec4 roughMetal = texture(metalRoughTex, inUV);
    roughness *= roughMetal.g;
    metalness *= roughMetal.b;
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   AMBIENT OCCLUSION   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // TODO TEST we could have many conditionals or we could default the textures
  // to a value that will preserve the conditions regardless so probably want to avoid branches
  // ?? even with the GPU
  float occlusionFactor = 1.0f;
  if ((materialData.flags & HasOcclusionTexture) == HasOcclusionTexture)
  {
    // Proper way to implement if material occlusion has associate strength factor but
    // if we are counting on the default strength factor (1.0) then reduces to used equation
    //occlusionFactor = 1.0 + materialData.occlusionFactor * (texture(occlusionMap, inUV).r - 1.0);
    occlusionFactor = texture(occlusionMap, inUV).r;
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EMISSION   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 emissive = vec3(0.f);
  if ((materialData.flags & HasEmissiveTexture) == HasEmissiveTexture)
  {
    emissive = texture(emissiveMap, inUV).rgb;
    emissive = decode_srgb(emissive) * vec3(materialData.emissiveFactors);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   NORMAL DISTRIBUTION   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#specular-brdf
  float alpha = roughness * roughness;
  float alphaSquared = alpha * alpha;

  float NdotH = dot(normalDirection, halfDirection);
  float NdotHsquared = max(0.0001, NdotH * NdotH);
  float distDenom = NdotHsquared * (alphaSquared - 1.0) + 1.0;
  float distribution = (alphaSquared * heaviside(NdotH)) / (PI * distDenom * distDenom);

  float NdotL = dot(normalDirection, lightDirection);
  //float NdotL = clamp( dot(normalDirection, lightDirection), 0.0, 1.0);
  //float NdotL = max( dot(normalDirection, lightDirection), 0.0);
  //  if (NdotL > 1e-5)
  // {
  float NdotV = dot(normalDirection, viewDirection);
  float HdotL = dot(halfDirection, lightDirection);
  float HdotV = dot(halfDirection, viewDirection);

  float visibility = heaviside(HdotL)
                     / (abs(NdotL) + sqrt(alphaSquared + (1.0 - alphaSquared) * (NdotL * NdotL)));
  visibility *= (heaviside(HdotV)
                 / (abs(NdotV) + sqrt(alphaSquared + (1.0 - alphaSquared) * (NdotV * NdotV))));

  float specularBRDF = visibility * distribution;

  vec3 diffuseBRDF = (1.0 / PI) * baseColor.rgb;// * baseColor.a;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float shadow = shadowCalculation(inPosLightSpace, preNormal, lightDirection);

  // // TODO calculate via input ambient color perhaps
  diffuseBRDF = diffuseBRDF * shadow;
  specularBRDF = specularBRDF * shadow;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FRESNEL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 conductorFresnel =  specularBRDF * (baseColor.rgb + (1.0 - baseColor.rgb) * pow(1.0 - abs(HdotV), 5));

//    float f0 = 0.04; // pow((1 - ior) / (1 + ior), 2);
  // TODO maybe even calc and pass in fr
  float fresnelRatio = materialData.iorF0 + (1.0 - materialData.iorF0) * pow(1.0 - abs(HdotV), 5);
  vec3 fresnelMix = mix(diffuseBRDF, vec3(specularBRDF), fresnelRatio);

  vec3 materialColor = mix(fresnelMix, conductorFresnel, metalness);

  // TODO pass this in
  materialColor = emissive +
                  mix(materialColor, materialColor * occlusionFactor, materialData.occlusionFactor);

  outFragColor = vec4(encode_srgb(materialColor), baseColor.a);
}

#define offsetBias 0.005


float shadowCalculation(vec4 posLightSpace, vec3 normal, vec3 lightDirection)
{
  // perform perspective divide (not necessary for orthographic projection but since w = 1 this is okay)
  vec3 coordsFromLight = posLightSpace.xyz / posLightSpace.w;

  // Make sure the shadow is not present when outside the far plane region of the lights frustrum
  if (coordsFromLight.z > 1.0)
  {
    return 0.0;
  }

  // transform NDC from [-1, 1] to [0,1] range since depthmap is in that range
  // note that coordsFromLight.z is alread in [0,1] range for Vulkan NDC
  coordsFromLight.x = coordsFromLight.x * 0.5 + 0.5;
  coordsFromLight.y = coordsFromLight.y * 0.5 + 0.5;

  // get closest depth value from light's perspective
  float maxLightReach = texture(shadowMap, coordsFromLight.xy).r;

  // calculate offset of depth in order to reduce shadow acne
//  vec3 lightDirection = vec3(0.0,0.0,0.0);//normalize(lightPos - inPosWorld);
  float depthBias = max(0.05 * (1.0 - dot(normal, lightDirection)), 0.005);

  // get depth of current fragment from light's perspective
  float fragDistanceFromLight = coordsFromLight.z - depthBias;

  // check whether current frag is in shadow or not
  //float shadow = maxLightReach < fragDistanceFromLight? 1.0 : 0.0;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PCF   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float shadow = 0.0;
  float ambientLight = 0.25;
  vec2 texelSize = 0.5 / textureSize(shadowMap, 0);

  // sample distance from the adjacent texels
  for(int x = -1; x <= 1; ++x)
  {
    for(int y = -1; y <= 1; ++y)
    {
      float maxLightReach = texture(shadowMap, coordsFromLight.xy + vec2(x, y) * texelSize).r;
      if (fragDistanceFromLight < maxLightReach)
      {
        shadow += 1.0;
      }
      else
      {
        shadow += ambientLight;
      }
      //shadow += fragDistanceFromLight > maxLightReach ? 1.0 : 0.0;
    }
  }
  // Take the average of all 9 sourounding texels
  shadow /= 9.0;
  return shadow;
}

// // Trowbridgel-Reitz/GGX distribution
//   //float roughness = materialData.MetalRoughFactors.y;
//   float roughnessSquared = materialData.MetalRoughFactors.y * materialData.MetalRoughFactors.y;
//   float dDenom = NdotH * NdotH * (roughnessSquared - 1.0f) + 1.0f;
//   float distribution = (roughnessSquared * step(0.0f, NdotH)) / (PI * dDenom * dDenom);


//   float visibility = (step(0.0f, HdotL)
//                       / (abs(NdotL) + sqrt(roughnessSquared + (1.0f - roughnessSquared) * (NdotV * NdotV))))
//                         * (step(0.0f, HdotV)
//                            / (abs(NdotV) + sqrt(roughnessSquared + (1.0f - roughnessSquared) * (NdotV * NdotV))));

//   float specularBRDF = visibility * distribution;

//   //vec3 intensity = sceneData.sunColor.xyz * sceneData.sunColor;
//   //vec3 color = inColor * texture(colorTex, inUV).xyz; // * sceneData.ambientColor.xyz;
//   vec3 color = materialData.colorFactors.xyz;

//   vec3 diffuseBRDF = (1.0f / PI) * color;
//   //vec3 diffuseBRDF = color / 4;

//   // f0 in the formula notation refers to the base color here
//   vec3 conductorFresnel = specularBRDF * (color + (1.0f - color) * pow(1.0f - abs(HdotV), 5));

//   // f0 in the formula notation refers to the value derived from ior = 1.5
//   float f0 = 0.04f; // pow((1 - ior) / (1 + ior), 2);
//   float fr = f0 + (1.0f - f0) * pow(1.0f - abs(HdotV), 5);
//   vec3 fresnelMix = mix(diffuseBRDF, vec3(specularBRDF), fr);

//   outFragColor = vec4(mix(fresnelMix, conductorFresnel, materialData.MetalRoughFactors.x), 1.0f);

//   vec3 FresnelFunction = specularColor + (1 - specularColor) * SchlickFresnel(LdotH);

//   vec3 specularity = (specularBRDF + FresnelFunction + GeometricShadow) / (4 * (NdotL * NdotV));

//   // Combine the
//   vec3 lightingModel = (diffuseColor + specularity);
//   lightingModel *= NdotL;
//   outFragColor = vec4(lightingModel * attenColor, 1.0f);

//   //vec3 color = inColor * texture(colorTex, inUV).xyz; // * sceneData.ambientColor.xyz;
