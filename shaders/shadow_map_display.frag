#version 450

layout(set = 0, binding = 0) uniform sampler2D shadowMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float zNear;
layout (location = 2) in float zFar;

layout (location = 0) out vec4 outFragColor;

float linearizeDepth(float depth);

void main()
{
  // Using a Sampler2D
  float depth = texture(shadowMap, inUV).r;
  outFragColor = vec4(vec3(1.0 - linearizeDepth(depth)), 1.0);
}

// TODO could possibly be pre-calculated in vertex shader
float linearizeDepth(float depth)
{
  // return (2.0 * zNear * zFar) / (zFar + zNear - depth * (zFar - zNear));
  return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}
