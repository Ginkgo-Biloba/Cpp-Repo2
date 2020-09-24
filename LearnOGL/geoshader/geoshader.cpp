#include "../images/shader.hpp"
#include <GLFW/glfw3.h>
#undef NDEBUG
#include <cassert>

static void errorCallback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
	fflush(stderr);
}

int main()
{
	int const winrows = 600;
	int const wincols = 800;

	glfwInit();
	glfwSetErrorCallback(errorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(wincols, winrows,
		"Geometry Shader Example", NULL, NULL);
	char const* err;
	glfwGetError(&err);
	assert(window);
	glfwMakeContextCurrent(window);
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	assert(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
	glViewport(0, 0, wincols, winrows);

	Shader shader("geo.vert", "geo.frag", "geo.geom");

	float points[] =
	{
		-0.5, +0.5, 1.0, 0.0, 0.0, // 左上
		+0.5, +0.5, 0.0, 1.0, 0.0, // 右上
		+0.5, -0.5, 0.0, 0.0, 1.0, // 左下
		-0.5, -0.5, 1.0, 1.0, 0.0, // 右下
	};

	unsigned vbo, vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
		reinterpret_cast<void*>(2 * sizeof(float)));

	glBindVertexArray(0);
	
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(0.1F, 0.3F, 0.2F, 1.F);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shader.ID);
		glBindVertexArray(vao);
		glDrawArrays(GL_POINTS, 0, 4);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glfwTerminate();
}
