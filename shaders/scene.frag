#version 450

uint HasColorTexture = 1 << 0;
uint HasNormalTexture = 1 << 1;
uint HasRoughMetalTexture = 1 << 2;
uint HasOcclusionTexture = 1 << 3;
uint HasEmissiveTexture = 1 << 4;
uint HasVertexTangentAttribute = 1 << 5;
uint HasVertexTextureCoordinates = 1 << 6;
//uint MAX_ENUM_FLAG = 0xFFFFFFFF;

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
layout(set = 2, binding = 0) uniform sampler2DShadow shadowMap;

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
// INPUT
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inTangent;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosWorld; // vertPosition
layout (location = 4) in vec4 inPosLightSpace;
// OUTPUT
layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265358;

// *-*-*-*-*-*-*-*-*-*-*-*-*-   FUNCTION DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
float shadowLookup(float x, float y);
// 4-sample pcf shadow calculation with offsets (dithering)
float ditheredShadowCalc();
vec3 getMappedNormal();
vec3 decode_srgb(vec3 color);
vec3 encode_srgb(vec3 color);
float distributionGGX(vec3 Normal, vec3 Half, float roughness);
float distributionGGX2(float NdotH, float alphaSquared);
float geometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness);
float geometrySchlickGGX(float NdotV, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);


void main()
{
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   ALBEDO   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec4 albedo = materialData.colorFactors;

  if ((materialData.flags & HasColorTexture) == HasColorTexture)
  {
    albedo = texture(colorTex, inUV);

    // Can choose either method of linearizing color
    // albedo.rgb = decode_srgb(texture(colorTex, inUV).rgb);
    albedo.rgb = pow(albedo.rgb, vec3(2.2));
  }

  if (albedo.a < 0.5)
  {
    discard;
  }


  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NORMAL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 normal;

  if ((materialData.flags & HasNormalTexture) == HasNormalTexture)
  {
    normal = getMappedNormal();
  }
  else
  {
    normal = normalize(inNormal);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   WORLD SPACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 viewDirection = normalize(sceneData.eye.xyz - inPosWorld);
  vec3 lightDirection = normalize(sceneData.sunDirection.xyz - inPosWorld);
  vec3 halfDirection = normalize(viewDirection + lightDirection);

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CHROME EFFECT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // // Get reflected view ray from the surface to calculate cube map reflection
  // //TODO do in one call
  // vec3 incident = -viewDirection;
  // vec3 reflection = reflect(incident, normalDirection);
  // vec4 reflectedColor = vec4(texture(skybox, reflection).rgb, 1.0);
  // outFragColor = reflectedColor;
  // return;

  // -*-*-*-*-*-*-*-*-*-*-*-*-   GLASS/WATER/ETC EFFECT   -*-*-*-*-*-*-*-*-*-*-*-*- //
  // float iorRatio = 1.00 / 1.309;
  // vec3 incident = -viewDirection;
  // vec3 refraction = refract(incident, normalDirection, iorRatio);
  // outFragColor = vec4(texture(skybox, refraction).rgb, 1.0);
  // return;

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   METALNESS / ROUGHNESS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float metalness = materialData.MetalRoughFactors.x;
  float roughness = materialData.MetalRoughFactors.y;

  if ((materialData.flags & HasRoughMetalTexture) == HasRoughMetalTexture)
  {
    vec4 roughMetal = texture(metalRoughTex, inUV);
    roughness *= roughMetal.g;
    metalness *= roughMetal.b;
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   AMBIENT OCCLUSION   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float occlusionFactor = 1.0;
  if ((materialData.flags & HasOcclusionTexture) == HasOcclusionTexture)
  {
    // ??
    // Proper way to implement if material occlusion has associate strength factor but
    // if we are counting on the default strength factor (1.0) then reduces to used equation
    //occlusionFactor = 1.0 + materialData.occlusionFactor * (texture(occlusionMap, inUV).r - 1.0);
    occlusionFactor = texture(occlusionMap, inUV).r;
  }

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EMISSION   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 emissive = vec3(0.0);
  if ((materialData.flags & HasEmissiveTexture) == HasEmissiveTexture)
  {
    emissive = texture(emissiveMap, inUV).rgb;
    emissive = decode_srgb(emissive) * vec3(materialData.emissiveFactors);
  }

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   COOK-TORRANCE BRDF   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // Calculate reflectance at normal incidence, if di-electric use F0 = 0.04
  // otherwise, if metal, us the albedo color as F0
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo.rgb, metalness);
  // float F0 = materialData.iorF0;
  // float HdotV = max(dot(halfDirection, viewDirection), 0.0);
  // float fresnelRatio = F0 + (1.0 - F0) * pow(1.0 - abs(HdotV), 5);


  // reflectance equation
  // ?? should this be a vec4 to account for alpha
  vec3 Lo = vec3(0.0);
  // here is where we would iterate in the case of multiple lights
  // for (int i = 0; i < NUM_LIGHTS; ++i) {


  float NdotH = max(dot(normal, halfDirection), 0.0);
  float alphaSquared = roughness * roughness * roughness * roughness;

  float NDF = distributionGGX2(NdotH, alphaSquared);
  float G = geometrySmith(normal, viewDirection, lightDirection, roughness);
  // vec3 Fresnel = fresnelSchlick(max(dot(halfDirection, viewDirection), 0.0), F0);
  vec3 Fresnel = fresnelSchlick(dot(halfDirection, viewDirection), F0);

  vec3 numerator = NDF * G * Fresnel;
  float denominator = 4.0 * max(dot(normal, viewDirection), 0.0);
  denominator *= max(dot(normal, lightDirection), 0.0);
  denominator += 0.0001; // bias to avoid divide by zero
  vec3 specular = numerator / denominator;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float shadow = 1.0 - ditheredShadowCalc();
  // TODO calculate via input ambient color perhaps
  // kS = kS * shadow;
  // kS is equal to Fresnel
  vec3 kS = Fresnel;

  // for energy conservation, the diffuse and specular light can't be above 1.0 (unless the
  // surface emits light). To preserve this relationship the diffuse component (kD) should be 1.0 - kS
  vec3 kD = vec3(1.0) - kS;
  // multipy kD by the inverse metalness such that only non-metals have diffuse lighting, or a
  // linear blend if partly metal (pure metals have no diffuse light).
  kD *= 1.0 - metalness;

  // Scale light by N dot L
  float NdotL = max(dot(normal, lightDirection), 0.0);

  // kD = kD * shadow;

// Add to outgoing radiance Lo (we already multiplied the BRDF by Fresnel (kS) so don't don't again)

  Lo += (kD * albedo.rgb / PI + specular) * NdotL * shadow;



  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   FINAL COLOR CALC   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //


  // Ambient lighting (note that the next IBL tutorial will replace with environment lighting)
  vec3 ambient = vec3(0.03) * albedo.rgb * occlusionFactor;

  vec3 color = ambient + emissive + Lo ;

  // HDR tonemapping
  color = color / (color + vec3(1.0));
  // Gamma correction
  // color = pow(color, vec3(1.0/2.2));
  color = encode_srgb(color);

  outFragColor = vec4(color, 1.0);
}


//
vec3 getMappedNormal()
{
  vec3 tangent;
  vec3 biTangent;
  mat3 TBN;

  vec3 normal = normalize(inNormal);
  // Normal textures are encoded to [0, 1] but need to be mapped to [-1, 1]
  vec3 tangentNormal = normalize(texture(normalMap, inUV).rgb * 2.0 -1.0);

  // *-*-*-*-*-*-*-*-*-   USING PASSED IN TANGENTS IF AVAILABLE   *-*-*-*-*-*-*-*-*- //
  if ((materialData.flags & HasVertexTangentAttribute) == HasVertexTangentAttribute)
  {
    tangent = normalize(inTangent.xyz);
    // ?? why mulitply by factor w
    // ?? shouldn't we normalize the biTangent
    biTangent = cross(tangentNormal, tangent) * inTangent.w;
    // biTangent = normalize(biTangent);
  }
  else // *-*-*-*-   COMPUTING THE TANGENT-SPACE W/O VERTEX ATTRIBUTE   *-*-*-*- //
  {
    //  https://community.khronos.org/t/computing-the-tangent-space-in-the-fragment-shader/52861
    vec3 Q1 = dFdx(inPosWorld);
    vec3 Q2 = dFdy(inPosWorld);
    vec2 st1 = dFdx(inUV);
    vec2 st2 = dFdy(inUV);

    tangent = normalize(Q1 * st2.t - Q2 * st1.t);
    biTangent = normalize(-Q1 * st2.s + Q2 * st1.s);
    // ?? equivalent??
    // biTangent = -normalize(cross(normal, tangent));
  }

  // the transpose of texture-to-eye space matrix
  TBN = mat3(tangent, biTangent, normal);
  return normalize(TBN * tangentNormal);
}


// *-*-*-*-*-*-*-   TROWBRIDGE-REITZ/GGX MICROFACET DISTRIBUTION   *-*-*-*-*-*-*- //
//
float distributionGGX(vec3 Normal, vec3 Half, float roughness)
{
  float alpha = roughness * roughness;
  float alphaSquared = alpha * alpha;
  float NdotH = max(dot(Normal, Half), 0.0);
  float NdotHsquared = NdotH * NdotH;

  float denom = (NdotHsquared * (alphaSquared - 1.0) + 1.0);
  denom = PI * denom * denom;

  // ?? Interestingly, this version does not employ the heaviside operator (maybe should try)
  return alphaSquared / denom;
  // return alphaSquared *  step(0.0, NdotH) / denom;
}


// *-*-*-*-*-*-*-   TROWBRIDGE-REITZ/GGX MICROFACET DISTRIBUTION   *-*-*-*-*-*-*- //
// TODO create a comparison flag that we can pass to the shaders for changing BRDF
float distributionGGX2(float NdotH, float alphaSquared)
{
  float denom = (NdotH * NdotH * (alphaSquared - 1.0) + 1.0);
  denom += 0.0001; // bias to avoid divide by zero
  denom = PI * denom * denom;

  // ?? Interestingly, this version does not employ the heaviside operator (maybe should try)
  // return alphaSquared / denom;
  return alphaSquared *  step(0.0, NdotH) / denom;
}


float geometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
  float NdotV = max(dot(normal, viewDirection), 0.0);
  float NdotL = max(dot(normal, lightDirection), 0.0);
  float ggx1 = geometrySchlickGGX(NdotL, roughness);
  float ggx2 = geometrySchlickGGX(NdotV, roughness);

  return ggx1 * ggx2;
}


// TODO rename NdotV since it can also be NdotL
float geometrySchlickGGX(float numer, float roughness)
{
  float r = roughness + 1.0;
  float k = r * r / 8.0;

  float denom = numer * (1.0 - k) + k;

  return numer / denom;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


// TODO get rid of normal and light direction dependencies
float ditheredShadowCalc()
{
  // vec3 coordsFromLight = inPosLightSpace.xyz / inPosLightSpace.w;
  // // float maxLightReach = texture(shadowMap, coordsFromLight.xy).r;
  // float depthBias = max(0.05 * (1.0 - dot(normal, lightDirection)), 0.005);
  // // get depth of current fragment from light's perspective
  // float fragDistanceFromLight = coordsFromLight.z + depthBias;

  float shadowFactor = 0.0;

  // Adjustable width for shadow dithering
  float shadowWidth = 0.5;
  // vec2 texelSize = 0.5 / textureSize(shadowMap, 0);

  // produces 1 of 4 possible samples (0,0), (0,1), (1,0) or (1,1) given current fragment coordinates
  vec2 offset = mod(floor(gl_FragCoord.xy), 2.0) * shadowWidth;

  shadowFactor += shadowLookup(-1.5 * shadowWidth + offset.x,  1.5 * shadowWidth - offset.y);
  shadowFactor += shadowLookup(-1.5 * shadowWidth + offset.x, -0.5 * shadowWidth - offset.y);
  shadowFactor += shadowLookup( 0.5 * shadowWidth + offset.x,  1.5 * shadowWidth - offset.y);
  shadowFactor += shadowLookup( 0.5 * shadowWidth + offset.x, -0.5 * shadowWidth - offset.y);

  // Take the average of the sampled points
  return shadowFactor / 4.0;
}


float shadowLookup(float x, float y)
{
  // // float maxLightReach = texture(shadowMap, vec2(x * 0.001 * inPosLightSpace.w,
  // //                                               y * 0.001 * inPosLightSpace.w)).r;
  // float ambientLight = 0.20;
  // vec2 texelSize = 0.5 / textureSize(shadowMap, 0);
  // float maxLightReach = texture(shadowMap, inPosLightSpace.xy + vec2(x, y) * texelSize).r;

  // // float tex = textureProj(shadowMap, inPosLightSpace + vec4(x * 0.001 * inPosLightSpace.w,
  // //                                                           y * 0.001 * inPosLightSpace.w,
  // //                                                           -0.01, 0.0));
  // if (fragDistanceFromLight > maxLightReach)
  // {
  //   return 1.0;
  // }
  // else
  // {
  //   return ambientLight;
  // }
  float tex = textureProj(shadowMap, inPosLightSpace + vec4(x * 0.001 * inPosLightSpace.w,
                                                            y * 0.001 * inPosLightSpace.w,
                                                            -0.01, 0.0));

  return tex;
}


//
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


//
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
