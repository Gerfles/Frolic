#version 450

 layout(set = 0, binding = 0) uniform sampler2D shadowMap;
//layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput shadowMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float zNear;
layout (location = 2) in float zFar;

layout (location = 0) out vec4 outFragColor;

float linearizeDepth(float depth);

void main()
{
  // Sanity check
  // if (materialData.flags == 0)
  // {
  //   outFragColor = vec4(0.7, 0.3, 0.5, 1.0);
  //   return;
  // }

  // vec3 uv = vec3(inUV, 0.3);
  // float depth = texture(shadowMap, uv);

  // Using a uniform subpassInput
  // float depth = subpassLoad(shadowMap).r;

  // Using a Sampler2D
  float depth = texture(shadowMap, inUV).r;
  outFragColor = vec4(vec3(1.0 - depth), 1.0);

  // TODO must linearize perspective projection

  // //float depth = subpassLoad(shadowMap).r;

  // if (inUV.x < 0.5)
  // {
  //   outFragColor = vec4(0.0,0.4,0.2, 1.0);
  // }
  // else
  // {
//  outFragColor = vec4(vec3(1.0 - linearizeDepth(depth)), 1.0);
  // if (depth == 1)
  // {

  //   //outFragColor = vec4(1.0, depth , 0.0, 1.0);
  //   //outFragColor = vec4(vec3(depth), 1.0);
  //   outFragColor = vec4(vec3(linearizeDepth(depth)), 1.0);
  // }
  // else if (depth < 1)
  // {
  //   //outFragColor = vec4(0.0,0.4,0.2, 1.0);
  //   outFragColor = vec4(0.0,0.0,1.0, 1.0);
  // }

  //outFragColor = vec4(vec3(linearizeDepth(depth)), 1.0);

//   if (depth == 0)
//   {
//     outFragColor = vec4(1.0,0.4,0.0, 1.0);
//   }
//   else if (depth == 1)
//   {
//     outFragColor = vec4(1.0, depth , 0.0, 1.0);
//   }
//   // else
//   // {
//   //outFragColor = vec4(vec3(depth), 1.0);
// //}
//   outFragColor = subpassLoad(shadowMap);
//}
//outFragColor = vec4(vec3(linearizeDepth(depth)), 1.0);

  //outFragColor = vec4(vec3(depth), 1.0);


//  outFragColor = vec4(inUV.x, inUV.y, 0.0, 1.0);
}

// TODO could possibly be pre-calculated in vertex shader
float linearizeDepth(float depth)
{
  // float z = depth * 2.0 - 1.0;
  // return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));
  return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}
