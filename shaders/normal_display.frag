// normal_display.frag
#version 450

layout (location = 0) in GEO_OUT
{
  vec3 normal;
  vec4 tangent;
  vec4 color;
} In;

layout (location = 0) out vec4 FragColor;

void main()
{
  FragColor = In.color;
}
