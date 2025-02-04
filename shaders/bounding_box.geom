// bounding_box.geom

#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 3) out;

void main()
{
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  // gl_Position = gl_in[0].gl_Position;
  // EmitVertex();

  EndPrimitive();
}
