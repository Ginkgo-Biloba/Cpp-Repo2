
#version 430 core

in vec3 ptcolor;
out vec4 fragcolor;

void main()
{
	fragcolor = vec4(ptcolor, 1.0);
}


