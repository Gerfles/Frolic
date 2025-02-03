// normal_display.geom

#version 450

void createLine(int index);

layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec4 invView;
  vec4 ambientColor;
  vec4 sunDirection;
  vec4 sunColor;
  //
} sceneData;

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec4 inTangent[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outTangent;
layout (location = 2) out vec3 outColor;

const float NORMAL_LENGTH = 0.1;


// TODO read from passed in normal map in here
// TODO compute tangent vectors in here
void main()
{
  // Generate a line from each vertex
  createLine(0);
  createLine(1);
  createLine(2);
}


void createLine(int index)
{
  vec3 color = vec3(1.0, 1.0, 0.0);
  if (index == 1)
  {
    color = vec3(1.0, 0.0, 1.0);
  }
  else if (index == 2)
  {
    color = vec3(0.0, 1.0, 1.0);
  }

  // start line at actual vertex
  gl_Position = sceneData.proj * gl_in[index].gl_Position;
  outTangent = inTangent[index];
  outNormal = inNormal[index];
  outColor = color;
  EmitVertex();

  // draw a line from vertex of NORMAL_LENGTH in the normal direction
  //gl_Position = gl_in[index].gl_Position + vec4(normalize(vec3(1,1,1)), 0.0) * NORMAL_LENGTH;
  gl_Position = sceneData.proj * (gl_in[index].gl_Position + vec4(inNormal[index], 0.0) * NORMAL_LENGTH);
  outTangent = inTangent[index];
  outNormal = inNormal[index];
  outColor = color;
  EmitVertex();

  EndPrimitive();
}
