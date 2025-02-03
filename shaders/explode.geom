// explode.geom

#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec4 inTangent[];
layout (location = 2) in vec2 inUV[];
layout (location = 3) in vec3 inPosWorld[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outTangent;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outPosWorld;


layout(push_constant) uniform constants
{
  // Not well documented but the offset here is because we can really only have
  // one push constant for the pipeline (command buffer) and within that push constant
  // we must rely on accessing specific ranges of it.
  layout(offset = 136)
  float expansionFactor;
} pushUniform;


void main()
{
  vec3 normal = normalize(inNormal[0] + inNormal[1] + inNormal[2]);
  normal.y = -normal.y;

  gl_Position = gl_in[0].gl_Position + vec4(normal * pushUniform.expansionFactor, 0.0);
  outUV = inUV[0];
  outPosWorld = inPosWorld[0];
  outTangent = inTangent[0];
  outNormal = inNormal[0];
  EmitVertex();

  gl_Position = gl_in[1].gl_Position + vec4(normal * pushUniform.expansionFactor, 0.0);
  outUV = inUV[1];
  outPosWorld = inPosWorld[1];
  outTangent = inTangent[1];
  outNormal = inNormal[1];
  EmitVertex();

  gl_Position = gl_in[2].gl_Position + vec4(normal * pushUniform.expansionFactor, 0.0);
  outUV = inUV[2];
  outPosWorld = inPosWorld[2];
  outTangent = inTangent[2];
  outNormal = inNormal[2];
  EmitVertex();

  EndPrimitive();
}
