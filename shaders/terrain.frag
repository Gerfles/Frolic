// terrain.frag
#version 450

//
layout (location = 0) in float inHeight;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in vec3 inEyePos;
layout (location = 6) in vec3 inWorldPos;
layout (location = 7) in float inTime;
//
layout (set = 0, binding = 2) uniform sampler2DArray terrainLayers;

layout (location = 0) out vec4 outFragColor;

vec3 sampleTerrainLayer();
float distanceFog(float density);
float volumeFog();

void main()
{
  // Uniform lighting based on height -> good for some forms of troubleshooting
  // float height = (inHeight + 16.0) / 64.0;
  // outFragColor = vec4(0.2, 1.0, 0.4, 1.0) * height;
  // outFragColor *= inColor;

  //
  vec3 N = normalize(inNormal);
  vec3 L = normalize(inLightVec);
  vec3 ambient = vec3(0.4);
  vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
  vec4 color = vec4((ambient + diffuse) * sampleTerrainLayer(), 1.0);

  // Simple fog based on distance
  const vec4 fogColor = vec4(0.44, 0.48, 0.63, 0.0);

  // Calculate the final color of the fragment, factoring in distance fog, and volumetric fog
  // vec4 terrainColor = mix(color, fogColor, distanceFog(0.25));
  // outFragColor = fogFactor * terrainColor + (1.0 - fogFactor) * fogColor;

  // Dynamic, volumetric Fog
  float fogFactor = volumeFog();

  // Disable distance fog
  outFragColor = fogFactor * color + (1.0 - fogFactor) * fogColor;
}



vec3 sampleTerrainLayer()
{
  const int NUM_LAYERS = 6;
  // Define the layers to sample based on terrain height
  vec2 layers[NUM_LAYERS];
  layers[0] = vec2(-10.0, 10.0);
  layers[1] = vec2(5.0, 45.0);
  layers[2] = vec2(45.0, 80.0);
  layers[3] = vec2(75.0, 100.0);
  layers[4] = vec2(95.0, 150.0);
  layers[5] = vec2(135.0, 190.0);

  float height = inHeight * 255.0;
  vec3 color = vec3(0.0);

  for(int i = 0; i < NUM_LAYERS; i++)
  {
    float range = layers[i].y - layers[i].x;
    float weight = (range - abs(height - layers[i].y)) / range;
    weight = max(0.0, weight);
    // ?? Not sure here where the factor of 16.0 comes from
    color += weight * texture(terrainLayers, vec3(inTexCoord * 16.0, i)).rgb;
  }
  return color;
}


float distanceFog(float density)
{
  const float LOG2 = -1.442695;
  float dist = distance(inEyePos, inWorldPos);// / gl_FragCoord.w;
  float d = density * dist * 0.04;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}


float volumeFog()
{



// TODO need to pass in values so we can change
  float slabY = 12.0;
  // TODO must seed this value with a time, etc.
  float startAngle = inTime/1.5;
  float oscillationAmplitude = 0.6;


  // Calculate the angles converted from the the vertex x and z coords
  float xAngle = inWorldPos.x / 16.0 * 3.1415926;
  float zAngle = inWorldPos.z / 20.0 * 3.1414926;

  // calculate the sine of the angle sum
  float slabYFactor = sin(xAngle + zAngle + startAngle) * oscillationAmplitude;

  // Find the t value for the inhtersecton of the ray equation p0+(p-p0)t and the
  // fog plane (p' - p) dot N = 0 from the fragment to the fog plane
  float t = (slabY + slabYFactor - inEyePos.y) / (inWorldPos.y - inEyePos.y);

  // The valid range of t should be [0,1] otherwise, means fragment is not below fog plane
  if (t > 0.0 && t < 1.0)
  {
    // Find coordinates of the intersection point of the ray and fog plane
    float xJD = inEyePos.x + (inWorldPos.x - inEyePos.x) * t;
    float zJD = inEyePos.z + (inWorldPos.z - inEyePos.z) * t;
    // float yJD = inEyePos.y + (inWorldPos.y - inEyePos.y) * t;
    vec3 locationJD = vec3(xJD, slabY, zJD);

    // Find the distance from the intesection to the position of the fragment to be processed
    float L = distance(locationJD, inWorldPos.xyz);
    if (inEyePos.y < slabY)
    {
      distance(locationJD, inEyePos);
    }

    float L0 = 8.0;

    // Calculate the fog concentration factor of volume fog
    return L0 / (L + L0);

  } else {
    // The fragment is below the fog plane, so will not be affected by the fog
    return 1.0;
  }
}
