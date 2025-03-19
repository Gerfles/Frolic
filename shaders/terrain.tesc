// Terrain Tesselation Control Shader
#version 450

// varying input from vertex shader
layout (location = 0) in vec2 inTexCoord[];
layout (location = 1) in vec3 inNormal[];
// specify number of control points per patch output, this value controls the size of the I/O arrays
layout (vertices = 4) out;

// varying output to tesselation evaluation shader
layout (location = 0) out vec2 outTexCoord[];
layout (location = 1) out vec3 outNormal[];

void main()
{
  // pass attributes through
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];

  // invocation zero controls tessellation levels for the entire patch
  if (gl_InvocationID == 0)
  {
    // Inner perimeter tesselation
    gl_TessLevelInner[0] = 16.0;
    gl_TessLevelInner[1] = 16.0;
    // Outer perimeter tesselation
    gl_TessLevelOuter[0] = 16.0;
    gl_TessLevelOuter[1] = 16.0;
    gl_TessLevelOuter[2] = 16.0;
    gl_TessLevelOuter[3] = 16.0;
  }
}
