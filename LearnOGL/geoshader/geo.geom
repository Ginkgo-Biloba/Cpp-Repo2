
#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices=5) out;

in VS_OUT { vec3 color; } vsout[];

out vec3 ptcolor;

void main()
{
	vec4 org = gl_in[0].gl_Position;
	// vsout[0] since there's only one input vertex
	ptcolor = vsout[0].color;

	gl_Position = org + vec4(-0.2, -0.2, 0, 0);
	EmitVertex();
	gl_Position = org + vec4(+0.2, -0.2, 0, 0);
	EmitVertex();
	gl_Position = org + vec4(-0.2, +0.2, 0, 0);
	EmitVertex();
	gl_Position = org + vec4(+0.2, +0.2, 0, 0);
	EmitVertex();
	
	ptcolor = vec3(1.0, 1.0, 1.0);
	gl_Position = org + vec4(+0.0, +0.4, 0, 0);
	EmitVertex();
	
	EndPrimitive();
}

