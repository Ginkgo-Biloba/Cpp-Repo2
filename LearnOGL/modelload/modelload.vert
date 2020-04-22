#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexture;

out vec2 TexCoords;

uniform mat4 model, view, proj;

void main()
{
	TexCoords = aTexture;
	gl_Position = proj * view * model * vec4(aPos, 1.0);
}
