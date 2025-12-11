// normal_display.geom
#version 450

void drawNormalFromVertex(int index);
void drawNormalFromCenter();

layout(std140, set = 0, binding = 0) uniform SceneData
{
  vec4 eye;
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  mat4 lightSpaceTransform;
  vec4 sunDirection;
  //
} sceneData;

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INPUTS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
layout (location = 0) in VS_OUT
{
  vec3 normal;
  vec4 tangent;
} In[];

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OUTPUTS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
layout (location = 0) out GEO_OUT
{
  vec3 normal;
  vec4 tangent;
  vec4 color;
} Out;

const float NORMAL_MAGNITUDE = 0.16;


// TODO read from passed in normal map in here
// TODO compute tangent vectors in here
void main()
{
  // *-*-*-*-*-*-*-*-*-*-   GENERATE A LINE FROM EACH VERTEX   *-*-*-*-*-*-*-*-*-*- //
  // drawNormalFromVertex(0);
  // drawNormalFromVertex(1);
  // drawNormalFromVertex(2);

  // -*-*-*-*-*-*-   ONLY DRAW A SINGLE NORMAL AT CENTER OF TRIANGLE   -*-*-*-*-*-*- //
  drawNormalFromCenter();
}

//
//
void drawNormalFromVertex(int index)
{
  // start at the polygon vertex
  vec3 startPoint = gl_in[index].gl_Position.xyz;
  // add normal direction and magnitude to starting point
  vec3 endPoint = startPoint + (In[index].normal * NORMAL_MAGNITUDE);

  // -*-*-*-*-*-*-*-*-*-*-*-   OUTPUT LINE START POSITION   -*-*-*-*-*-*-*-*-*-*-*- //
  gl_Position = sceneData.viewProj * vec4(startPoint, 1.0);
  Out.tangent = In[index].tangent;
  Out.normal = In[index].normal;

  // use color based on normal direction
  vec4 color = vec4(abs(In[index].normal), 1.0);
  Out.color = color;
  EmitVertex();

  // *-*-*-*-*-*-*-*-*-*-*-*-*-   OUTPUT LINE ENDPOINT   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  gl_Position = sceneData.viewProj * vec4(endPoint, 1.0);
  Out.tangent = In[index].tangent;
  Out.normal = In[index].normal;

  // fade out towards the tip of the normal vector
  color.a = 0.25;
  Out.color = color;
  EmitVertex();

  EndPrimitive();
}

//
//
void drawNormalFromCenter()
{
  vec3 averagedNormals = (In[0].normal + In[1].normal + In[1].normal) / 3;

  // original vertex positions
  vec3 v0 = gl_in[0].gl_Position.xyz;
  vec3 v1 = gl_in[1].gl_Position.xyz;
  vec3 v2 = gl_in[2].gl_Position.xyz;
  // original normal vectors
  vec3 n0 = v0 + In[0].normal * NORMAL_MAGNITUDE;
  vec3 n1 = v1 + In[1].normal * NORMAL_MAGNITUDE;
  vec3 n2 = v2 + In[2].normal * NORMAL_MAGNITUDE;

  vec3 startPoint = (v0 + v1 + v2) / 3;
  vec3 endPoint = startPoint + (averagedNormals * NORMAL_MAGNITUDE);

  // *-*-*-*-*-*-*-*-*-*-*-   OUTPUT LINE STARTING POSITION   *-*-*-*-*-*-*-*-*-*-*- //
  gl_Position = sceneData.viewProj * vec4(startPoint, 1.0);
  // TODO must weight the normals and tangents etc. or disregard
  Out.tangent = In[0].tangent;
  Out.normal = In[0].normal;

  vec4 color = vec4(abs(averagedNormals), 1.0);
  Out.color = color;
  EmitVertex();

  // *-*-*-*-*-*-*-*-*-*-*-*-*-   OUTPUT LINE END POINT   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  gl_Position = sceneData.viewProj * vec4(endPoint, 1.0);

  Out.tangent = In[0].tangent;
  Out.normal = In[0].normal;

  // fade out towards the tip of the normal vector
  color.a = 0.25;
  Out.color = color;
  EmitVertex();

  EndPrimitive();
}
