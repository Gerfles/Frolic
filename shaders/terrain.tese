// Terrain tessellation evaluation shader
#version 450

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INPUTS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
layout (quads, equal_spacing, cw) in;

// TODO might want to consider a smaller uniform specific to terrain
// ?? Learn the best practices surrounding this (probably want to break it up into multiple uniforms based on always used vs. somtimes used)

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

layout(set = 0, binding = 1) uniform sampler2D heightMap;

// Input texture coordinates from tessellation control shader
layout (location = 0) in vec2 inTexCoord[];
layout (location = 1) in vec3 inNormal[];

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OUTPUT   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// output height to frag shader for coloring
layout (location = 0) out float height;
layout (location = 1) out vec2 outTexCoord;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec3 outEyePos;
layout (location = 6) out vec3 outWorldPos;


layout(push_constant, std430) uniform constants
{
  // Not well documented but the offset here is because we can really only have
  // one push constant for the pipeline (command buffer) and within that push constant
  // we must rely on accessing specific ranges of it.
  layout (offset = 16) mat4 model;
} push;


void main()
{
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   INTERPOLATE TEXTURE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec2 texCoord1 = mix(inTexCoord[0], inTexCoord[1], gl_TessCoord.x);
  vec2 texCoord2 = mix(inTexCoord[3], inTexCoord[2], gl_TessCoord.x);
  outTexCoord = mix(texCoord1, texCoord2, gl_TessCoord.y);

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   INTERPOLATE NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec3 norm1 = mix(inNormal[0], inNormal[1], gl_TessCoord.x);
  vec3 norm2 = mix(inNormal[3], inNormal[2], gl_TessCoord.x);
  outNormal = mix(norm1, norm2, gl_TessCoord.y);

// *-*-*-*-*-*-*-*-*-*-*-*-*-   INTERPOLATE POSITION   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
  vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
  vec4 position = mix(pos1, pos2, gl_TessCoord.y);

  // Displace
  height = textureLod(heightMap, outTexCoord, 0.0).r;
  position.y += height * ubo.displacementFactor;

  gl_Position = ubo.modelViewProj * position;

  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LIGHTING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

  outViewVec = -position.xyz;
  outWorldPos = position.xyz;
  outEyePos = vec3(ubo.modelView * position);
  outLightVec = normalize(ubo.lightPos.xyz + outViewVec);
  // outLightVec = normalize(ubo.lightPos.xyz - outWorldPos);

  // Texture interpolation
  // // get patch coordinate
  // float u = gl_TessCoord.x;
  // float v = gl_TessCoord.y;

  // // retrieve control point texture coordinates
  // vec2 t00 = inTexCoord[0];
  // vec2 t01 = inTexCoord[1];
  // vec2 t10 = inTexCoord[2];
  // vec2 t11 = inTexCoord[3];

  // // bilinearly interpolate texture coordinate across patch
  // vec2 t0 = (t01 - t00) * u + t00;
  // vec2 t1 = (t11 - t10) * u + t10;
  // vec2 outTexCoord = (t1 - t0) * v + t0;

  // lookup texel at patch coordinate for height and scale + shift as desired
  // height = texture(heightMap, outTexCoord).y * 64.0 - 16.0;

  // // retrieve control point position coordinates
  // vec4 p00 = gl_in[0].gl_Position;
  // vec4 p01 = gl_in[1].gl_Position;
  // vec4 p10 = gl_in[2].gl_Position;
  // vec4 p11 = gl_in[3].gl_Position;

  // // compute patch surface normal
  // vec4 uVec = p01 - p00;
  // vec4 vVec = p10 - p00;
  // vec4 normal = normalize(vec4(cross(vVec.xyz, uVec.xyz), 0.0));

  // // bilinearly interpolate position coordinate across patch
  // vec4 p0 = (p01 - p00) * u + p00;
  // vec4 p1 = (p11 - p10) * u + p10;
  // vec4 p = (p1 - p0) * v + p0 + normal * height;

  // // displace point along normal
  // // p += normal * height;

  // // outpu patch point position in clip space
  // gl_Position = scene.viewProj * push.model * p;
}
