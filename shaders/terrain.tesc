// Terrain Tesselation Control Shader
#version 450

// varying input from vertex shader
layout (location = 0) in vec2 inTexCoord[];
layout (location = 1) in vec3 inNormal[];

// specify number of control points per patch output, this value controls the size of the I/O arrays
layout (vertices = 4) out;
layout (location = 0) out vec2 outTexCoord[];
layout (location = 1) out vec3 outNormal[];

layout(std140, set = 0, binding = 0) uniform UBO
{
  mat4 projection;
  mat4 modelView;
  mat4 modelViewProj;
  vec4 lightPos;
  vec4 frustumPlanes[6];
  float displacementFactor;
  float tessellationFactor;
  vec2 viewportDim;
  float tessellatedEdgeSize;
} ubo;

// DELETE don't believe its needed here without using sascha method
layout(set = 0, binding = 1) uniform sampler2D heightMap;

bool checkFrustum();
float calcTessellationFactor(vec4 p0, vec4 p1);
float distFromCamera(vec4 point);

const int MIN_TESS_LEVEL = 1;
const int MAX_TESS_LEVEL = 32;
// This would be the minimum distance to reduce tessellation, anything closer
// than that will automatically be at max tessellation level
const float MIN_DISTANCE = 15.0;
const float MAX_DISTANCE = 60.0;

void main()
{
  // pass attributes through
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
  outNormal[gl_InvocationID] = inNormal[gl_InvocationID];

  // invocation zero controls tessellation levels for the entire patch
  if (gl_InvocationID == 0)
  {
    if (!checkFrustum())
    { // Inner perimeter
      gl_TessLevelInner[0] = 0.0;
      gl_TessLevelInner[1] = 0.0;
      // Outer perimeter
      gl_TessLevelOuter[0] = 0.0;
      gl_TessLevelOuter[1] = 0.0;
      gl_TessLevelOuter[2] = 0.0;
      gl_TessLevelOuter[3] = 0.0;
    }
    else
    {
        /*   QUAD TESSELLATION MAP (ACCORDING TO SPECS)
     P0(0, 0)___________OL[1]__________P3(1,0)
            |                          |
            |     ________________     |
            |    |      IL[0]     |    |
            |    |                |    |
      OL[0] |    | IL[1]     IL[1]|    | OL[2]
            |    |                |    |
            |    |      IL[0]     |    |
            |    |________________|    |
            |                          |
            |__________________________|
     P1(0,1)           OL[3]            P2(1,1)
      */

  /*   QUAD TESSELLATION MAP (FOUND BY TWEAKING)
     P0(0, 0)___________OL[O]__________P3(1,0)
            |                          |
            |     ________________     |
            |    |      IL[0]     |    |
            |    |                |    |
      OL[1] |    | IL[1]     IL[1]|    | OL[3]
            |    |                |    |
            |    |      IL[0]     |    |
            |    |________________|    |
            |                          |
            |__________________________|
     P1(0,1)           OL[2]            P2(1,1)
      */

      // ?? Not sure if the point location is implementation specific but didn't
      // find much info in specs
      float distance00 = distFromCamera(gl_in[0].gl_Position);
      float distance01 = distFromCamera(gl_in[1].gl_Position);
      float distance10 = distFromCamera(gl_in[3].gl_Position);
      float distance11 = distFromCamera(gl_in[2].gl_Position);

      float tessLevel0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, max(distance10, distance00));
      float tessLevel1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, max(distance00, distance01));
      float tessLevel2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, max(distance01, distance11));
      float tessLevel3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, max(distance11, distance10));

      gl_TessLevelOuter[0] = tessLevel0;
      gl_TessLevelOuter[1] = tessLevel1;
      gl_TessLevelOuter[2] = tessLevel2;
      gl_TessLevelOuter[3] = tessLevel3;

      // TEST to make sure the inner levels are at proper locations
      gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
      gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);

      // SASCHA METHOD Doesn't seem to work well close up as the tess levels
      // change drastically based on your viewing angle
      // Outer perimeter tesselation
      // gl_TessLevelOuter[0] = calcTessellationFactor(gl_in[3].gl_Position
      //                                               , gl_in[0].gl_Position);
      // gl_TessLevelOuter[1] = calcTessellationFactor(gl_in[0].gl_Position
      //                                               , gl_in[1].gl_Position);
      // gl_TessLevelOuter[2] = calcTessellationFactor(gl_in[1].gl_Position
      //                                               , gl_in[2].gl_Position);
      // gl_TessLevelOuter[3] = calcTessellationFactor(gl_in[2].gl_Position
      //                                               , gl_in[3].gl_Position);
      // // Inner perimeter tesselation
      // gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
      // gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
    }
  }
}

// Checks the current patch visibility against the frustum using a sphere check
bool checkFrustum()
{
  // TODO don't hard-code
  // Sphere radius is set based on patch size (increase if numPatches is increased)
  const float radius = 6.0f; // 8.0f;
  // TODO test if we could set the displacement factor within generateTerrain() by setting
  // position.y to the displacement factor and *= instead, since it doesn't ever seem like
  // we would have a need to alter the displacement factor once map is set??
  vec4 pos = gl_in[gl_InvocationID].gl_Position;
  pos.y += textureLod(heightMap, inTexCoord[0], 0.0).r * ubo.displacementFactor;

  // check sphere against frustum planes
  for(int i = 0; i < 6; i++)
  {
    if(dot(pos, ubo.frustumPlanes[i]) + radius < 0.0)
    {
      return false;
    }
  }
  return true;
}


// Calculate the tessellation factor based on screen space dimensions of the edge
float calcTessellationFactor(vec4 p0, vec4 p1)
{
  // calculate edge mid point
  vec4 midPoint = 0.5 * (p0 + 01);
  // Sphere radius as distance between the control points
  float radius = distance(p0, p1) / 2.0;

  // project midpoint into view space
  vec4 v0 = ubo.modelView * midPoint;

  // temp
  //ubo.projection[1][1] *= -1;

  // project into clip space
  vec4 clip0 = (ubo.projection * (v0 - vec4(radius, vec3(0.0))));
  vec4 clip1 = (ubo.projection * (v0 + vec4(radius, vec3(0.0))));

  // Convert to normalized device coordinates
  clip0 /= clip0.w;
  clip1 /= clip1.w;

  // Convert to viewport coordinates
  clip0.xy *= ubo.viewportDim;
  clip1.xy *= ubo.viewportDim;
  //  return 16.0f;

  // Return the tessellation factor based on the screen size given by
  // the distance of the two edge control points in screen space and a
  // reference (min.) tessellation size for the edge set by application
  return clamp(distance(clip0, clip1) / ubo.tessellatedEdgeSize * ubo.tessellationFactor
               , MIN_TESS_LEVEL, MAX_TESS_LEVEL);
}


float distFromCamera(vec4 point)
{
  // Project control points into view space
  vec4 viewSpacePos = ubo.modelView * point;

  float ret = clamp((length(viewSpacePos.zyx) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

  // float ret = clamp((abs(viewSpacePos.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

  return ret;
}
