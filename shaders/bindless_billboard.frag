// bindless_billboard.frag
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// use GLSL version 4.5
#version 450

// Required to use bindless textures
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 texCoords;

// billboard specific parameters
  layout(push_constant) uniform Push
  {
    vec3 position;
    float width;
    float height;
    uint texIndex;
  } push;

//
layout(set = 1, binding = 10) uniform sampler2D globalTextures[];
layout(set = 1, binding = 10) uniform sampler3D globalTextures3D[];

layout(location = 0) out vec4 outColor; // final output color (out and in are different locations)


void main()
{
  outColor = texture(globalTextures[nonuniformEXT(push.texIndex)], texCoords);

  // if (outColor.a < 0.5)
  // {
  //   discard;
  // }

  //outColor = vec4(0.0, 0.0, 1.0, 1.0);
  //outColor = fragColor;
  //outColor = vec4(push.color.xyz, 1.0);
}
