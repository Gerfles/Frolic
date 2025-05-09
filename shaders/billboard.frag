// billboard.frag
// use GLSL version 4.5
#version 450

layout(location = 0) in vec2 texCoords;

// struct Billboard
// {
//    vec4 position; // ignore w
//    vec4 color; // w is intensity
// };

// struct PointLight
// {
//    vec4 position; // ignore w
//    vec4 color; // w is intensity
// };

// this actually isn't supplied by the pipeline to the fragment shader
// billboard specific parameters
// layout(push_constant) uniform Push
// {
//   vec4 position;
//   vec4 color;
//   float width;
//   float height;
// } push;


layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor; // final output color (out and in are different locations)


void main()
{
  outColor = texture(textureSampler, texCoords);
  // if (outColor.a < 0.5)
  // {
  //   discard;
  // }

  //outColor = vec4(0.0, 0.0, 1.0, 1.0);
  //outColor = fragColor;
  //outColor = vec4(push.color.xyz, 1.0);
}
