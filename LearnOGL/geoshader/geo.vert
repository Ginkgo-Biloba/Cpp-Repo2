
#version 430 core

layout (location=0) in vec2 apos;
layout (location=1) in vec3 color;
out VS_OUT { vec3 color; } vsout;

void main()
{
	gl_Position = vec4(apos.x, apos.y, 0.0, 1.0);
	vsout.color = color;
}
