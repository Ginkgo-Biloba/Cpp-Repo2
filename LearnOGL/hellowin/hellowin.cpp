// https://learnopengl-cn.github.io/01%20Getting%20started/03%20Hello%20Window/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>

#pragma comment(lib, "glfw3.lib")

static int const WinWidth = 800;
static int const WinHeight = 600;


// process all input
// query GLFW whether relevant keys are pressed/released this frame and react accordingly
void frame_size_change(GLFWwindow* wd, int w, int h)
{
	glViewport(0, 0, w, h);
}


// glfw: whenever the window size changed (by OS or user resize) 
// this callback function executes
void proc_input(GLFWwindow* wd)
{
	if (glfwGetKey(wd, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(wd, 1);
}


int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* wd = glfwCreateWindow(WinWidth, WinHeight, "Hello Window", NULL, NULL);
	if (wd == NULL)
	{
		printf("Fail to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(wd);
	glfwSetFramebufferSizeCallback(wd, frame_size_change);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		printf("Fail to initialize GLAD\n");
		return -2;
	}

	// render loop
	while (!glfwWindowShouldClose(wd))
	{
		// input
		proc_input(wd);

		// render
		glClearColor(0.2F, 0.3F, 0.3F, 1.F);
		glClear(GL_COLOR_BUFFER_BIT);

		// glfw: swap buffers and poll IO events 
		// (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(wd);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
