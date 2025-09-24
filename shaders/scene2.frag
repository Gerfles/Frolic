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
const vec3 BLACK = vec3(0.0);

// *-*-*-*-*-*-*-*-*-*-*-*-*-   FUNCTION DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
float shadowLookup(float x, float y);
// 4-sample pcf shadow calculation with offsets (dithering)
float ditheredShadowCalc();
float PCFshadowCalc();
vec3 getMappedNormal();
vec3 decode_srgb(vec3 color);
vec3 encode_srgb(vec3 color);
float visibility(float HdotL, float HdotV, float NdotL, float NdotV, float alphaSquared);
float distributionGGX(float NdotH, float alphaSquared);
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

  // vec3 Lo = vec3(0.0);
  // here is where we would iterate in the case of multiple lights
  // for (int i = 0; i < NUM_LIGHTS; ++i) {

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   WORLD SPACE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 viewDirection = normalize(sceneData.eye.xyz - inPosWorld);
  vec3 lightDirection = normalize(sceneData.sunDirection.xyz - inPosWorld);
  vec3 halfDirection = normalize(viewDirection + lightDirection);

  // -*-*-*-*-*-*-*-*-*-*-*-   COOK-TORRANCE BRDF FACTORS   -*-*-*-*-*-*-*-*-*-*-*- //
  float alphaSquared = roughness * roughness * roughness * roughness;
  float HdotL = max(dot(halfDirection, lightDirection), 0.0);
  float HdotV = max(dot(halfDirection, viewDirection), 0.0);
  float NdotL = max(dot(normal, lightDirection), 0.0);
  float NdotV = max(dot(normal, viewDirection), 0.0);
  float NdotH = max(dot(normal, halfDirection), 0.0);

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FRESNEL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // TODO make sure all glTFs pass in at least a default value of 0.04
  // Calculate reflectance at normal incidence, if di-electric use F0 = 0.04
  // otherwise, if metal, us the albedo color as F0

  // vec3 met = mix(vec3(0.0), albedo.rgb, metalness);
  // vec3 color_diff = albedo.rgb - met;

  vec3 color_diff = mix(albedo.rgb, BLACK, metalness);
  // F0 == reflection color at normal incidence (typically 0.04 for a dielectric)
  vec3 F0 = vec3(materialData.iorF0);
  // Metal will have an F0 equal to the material color
  F0 = mix(F0, albedo.rgb, metalness);

  vec3 Fresnel = fresnelSchlick(HdotV, F0);

  // // TRY this different method
  // // Metalic brdf: F0 = baseColor, bsdf = specular_brdf(a = roughness^2)
  // vec3 conductorFresnel = specular * fresnelSchlick(HdotV, F0);
  // // dielectric brdf: default ior = 1.5, base = diffuse_brdf(color = albedo),
  // // layer = specular_brdf(a = roughness^2)

  // *-*-*-*-*-*-*-*-*-*-*-*-   SPECULAR REFLECTION BRDF   *-*-*-*-*-*-*-*-*-*-*-*- //
  float distribution = distributionGGX(NdotH, alphaSquared);
  float visibility = visibility(HdotL, HdotV, NdotL, NdotV, alphaSquared);

  vec3 specular = Fresnel * distribution * visibility;

  // -*-*-*-*-*-*-*-*-*-*-*-*-   DIFFUSE REFLECTION BRDF   -*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 diffuse = (1.0 - Fresnel) * (1.0 / PI) * color_diff;

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW   *-*-*-*-*-u*-*-*-*-*-*-*-*-*-*-*-*- //
  // TODO determine how to make shadows darker and without darkening scene where no shadow
  // should exist
  float shadow = ditheredShadowCalc();
  // float shadow = 1.0 - PCFshadowCalc();

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FINAL COLORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 material =  specular + diffuse;

  // Ambient lighting (note that the next IBL tutorial will replace with environment lighting)
  vec3 ambient = vec3(0.03) * albedo.rgb * occlusionFactor;


  vec3 color = ambient + emissive + (shadow * material * NdotL);

  // -*-*-*-*-*-*-*-*-*-*-*-*-   OUTPUT COLOR CONVERSION   -*-*-*-*-*-*-*-*-*-*-*-*- //
  // HDR tonemapping
  color = color / (color + vec3(1.0));
  // Gamma correction
  color = pow(color, vec3(1.0/2.2));
  // color = encode_srgb(color);
  outFragColor = vec4(color, albedo.a);
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
float distributionGGX(float NdotH, float alphaSquared)
{
  float denom = (NdotH * NdotH * (alphaSquared - 1.0) + 1.0);
  denom += 0.0001; // bias to avoid divide by zero
  denom = PI * denom * denom;

  // ?? Interestingly, this version does not employ the heaviside operator (maybe should try)
  // return alphaSquared / denom;
  return alphaSquared *  step(0.0, NdotH) / denom;
}


// *-*-*-*-*-*-*-   TROWBRIDGE-REITZ/GGX MICROFACET DISTRIBUTION   *-*-*-*-*-*-*- //
// TODO create a comparison flag that we can pass to the shaders for changing BRDF
float distributionGGX2(vec3 Normal, vec3 Half, float roughness)
{
  float alpha = roughness * roughness;
  float alphaSquared = alpha * alpha;
  float NdotH = max(dot(Normal, Half), 0.0);
  float NdotHsquared = NdotH * NdotH;

  float denom = (NdotHsquared * (alphaSquared - 1.0) + 1.0);

  denom = PI * denom * denom;

  // ?? Interestingly, this version does not employ the heaviside operator (maybe should try)
  return alphaSquared *  step(0.0, NdotH) / denom;
}


// Visibility is G / 4|(NdotL)||(NdotV)|
float visibility(float HdotL, float HdotV, float NdotL, float NdotV, float alphaSquared)
{
  float numer = step(0.0, HdotL) * step(0.0, HdotV);

  // TODO check for speed
  // if (numer == 0)
  // {
  //   return 0.0;
  // }

  float denom = abs(NdotL) + sqrt(alphaSquared + (1 - alphaSquared) * NdotL * NdotL);
  denom *= abs(NdotV) + sqrt(alphaSquared + (1 - alphaSquared) * NdotV * NdotV);
  denom += 0.0001; // bias to avoid divide by zero

  return numer / denom;
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
float geometrySchlickGGX(float NdotV, float roughness)
{
  float r = roughness + 1.0;
  float k = r * r / 8.0;

  float denom = NdotV * (1.0 - k) + k;

  return NdotV / denom;
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
  float shadowWidth = 0.05;
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




float PCFshadowCalc()
{
  // Make sure the shadow is not present when outside the far plane region of the lights frustrum
  // if (coordsFromLight.z > 1.0)
  // {
  //   return 0.0;
  // }

  // ?? may not need if multiplying by bias transform matrix
  // transform NDC from [-1, 1] to [0,1] range since depthmap is in that range
  // note that coordsFromLight.z is alread in [0,1] range for Vulkan NDC
  // coordsFromLight.x = coordsFromLight.x * 0.5 + 0.5;
  // coordsFromLight.y = coordsFromLight.y * 0.5 + 0.5;

  // get closest depth value from light's perspective
  // float maxLightReach = texture(shadowMap, coordsFromLight.xy).r;

  // calculate offset of depth in order to reduce shadow acne
  //  vec3 lightDirection = vec3(0.0,0.0,0.0);//normalize(lightPos - inPosWorld);

  // get depth of current fragment from light's perspective
  // float fragDistanceFromLight = coordsFromLight.z + depthBias;

  // check whether current frag is in shadow or not
  //float shadow = maxLightReach < fragDistanceFromLight? 1.0 : 0.0;


  // TODO Develop alternative pipelines that use different shaders for enabling/disabling features
  // That way we can implement all the features but decide at compile time and avoid branches
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PCF   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  float shadowFactor = 0.0;
  // float ambientLight = 0.25;
  vec2 texelSize = 0.5 / textureSize(shadowMap, 0);
  // vec2 texelSize = vec2(0.005, 0.005);

  // sample distance from the adjacent texels
  for(int x = -1; x <= 1; ++x)
  {
    for(int y = -1; y <= 1; ++y)
    {
      // FIXME to make work with sampler2DShadow
      // float maxLightReach = shadowLookup(x, y);
      shadowFactor += shadowLookup(x * texelSize.x, y * texelSize.y);
      // * texelSize).r;
      //shadowFactor += (fragDistanceFromLight > maxLightReach) ? 1.0 : ambientLight;
    }
  }
  // Take the average of all 9 sourounding texels
  return shadowFactor / 9.0;
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
