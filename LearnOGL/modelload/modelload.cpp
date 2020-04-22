#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include "../images/model.hpp"


unsigned const WIDTH = 800;
unsigned const HEIGHT = 600;

Camera camera(glm::vec3(0, 0, 3));
float lastX = WIDTH * 0.5F;
float lastY = HEIGHT * 0.5F;
bool first_mouse = true;

float deltaTime = 0;
float lastFrame = 0;


void cb_framebuffer(GLFWwindow* win, int cols, int rows)
{
	// make sure the viewport matches the new window dimensions
	// note that width and height will be significantly larger than specified on retina displays.
	glViewport(0, 0, cols, rows);
}


void cb_mouse(GLFWwindow* win, double x0, double y0)
{
	float x = static_cast<float>(x0);
	float y = static_cast<float>(y0);
	if (first_mouse)
	{
		lastX = x;
		lastY = y;
		first_mouse = 0;
	}
	// reversed since y-coordinates go from bottom to top
	float dx = x - lastX;
	float dy = lastY - y;
	lastX = x;
	lastY = y;
	camera.ProcessMouseMovement(dx, dy);
}


void cb_scroll(GLFWwindow* win, double, double y0)
{
	camera.ProcessMouseScroll(static_cast<float>(y0));
}


void cb_process(GLFWwindow* win)
{
	if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(win, 1);
	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
	if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::LEFT, deltaTime);
	if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
}


int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	// uncomment this statement to fix compilation on OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
#endif

	GLFWwindow* win = glfwCreateWindow(WIDTH, HEIGHT, "Model Load", NULL, NULL);
	if (win == NULL)
	{
		fprintf(stderr, "failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(win);
	glfwSetFramebufferSizeCallback(win, cb_framebuffer);
	glfwSetCursorPosCallback(win, cb_mouse);
	glfwSetScrollCallback(win, cb_scroll);
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		return -2;
	}
	
	glEnable(GL_DEPTH_TEST);

	Shader sdr("modelload.vert", "modelload.frag");

	Model mdl("../nanosuit/nanosuit.obj");

	// draw in wireframe
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	while (!glfwWindowShouldClose(win))
	{
		float curFrame = static_cast<float>(glfwGetTime());
		deltaTime = curFrame - lastFrame;
		lastFrame = curFrame;

		cb_process(win);

		glClearColor(0.05F, 0.06F, 0.05F, 1.F);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		sdr.use();

		// view/projection transformations
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom),
			1.F * WIDTH / HEIGHT, 0.1F, 100.F);
		glm::mat4 view = camera.GetViewMatrix();
		sdr.setMat4("proj", proj);
		sdr.setMat4("view", view);

		// render the loaded model
		glm::mat4 model = glm::mat4(1.F);
		// translate it down so it's at the center of the scene
		model = glm::translate(model, glm::vec3(0.F, -1.75F, 0.F));
		// it's a bit too big for our scene, so scale it down
		model = glm::scale(model, glm::vec3(0.2F, 0.2F, 0.2F));
		sdr.setMat4("model", model);
		
		mdl.Draw(sdr);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
