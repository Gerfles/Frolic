#version 450


layout (location = 0) in float inHeight;
layout (location = 1) in vec2 inTexCoord;


layout (location = 0) out vec4 fragColor;

void main()
{
  // Shift and scale the height into a grayscale value
  float height = (inHeight + 16.0) / 64.0;

  fragColor = vec4(height, height, height, 1.0);
  //fragColor = vec4(0.1, 0.8, 0.4, 1.0);

//  fragColor = inColor;
}
