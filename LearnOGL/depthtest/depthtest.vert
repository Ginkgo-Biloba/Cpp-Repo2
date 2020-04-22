#version 430 core

layout(location=0) in vec3 pos;
layout(location=1) in vec2 tex;

out vec2 texcd;

uniform mat4 model, view, proj;

void main()
{
	texcd = tex;
	gl_Position = proj * view * model * vec4(pos, 1.0);
}
