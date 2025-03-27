#version 450


layout (location = 0) in float inHeight;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 outNormal;
layout (location = 3) in vec4 color;



layout (location = 0) out vec4 fragColor;

void main()
{
  // Shift and scale the height into a grayscale value
  float height = (inHeight + 16.0) / 64.0;


  fragColor = vec4(0.2, 1.0, 0.4, 1.0) * height;

  //fragColor = vec4(0.2, height, 0.4, 1.0);
    fragColor *= color;

  //fragColor = vec4(0.1, 0.8, 0.4, 1.0);

//  fragColor = inColor;
}
