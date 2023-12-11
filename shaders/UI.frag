// UI FRAGMENT SHADER
// use GLSL version 4.5
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 texCoords;

// struct Billboard
// {
//    vec4 position; // ignore w
//    vec4 color; // w is intensity
// };


layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor; // final output color (out and in are different locations)


void main()
{
//  outColor = vec4(1.0, 1.0, 1.0, 1.0);
  outColor = texture(textureSampler, texCoords);
  // if (outColor.w < 0.3)
  // { 
  //  outColor = outColor.w + vec4(1.0, 1.0, 1.0, 1.0);
  // }
}
