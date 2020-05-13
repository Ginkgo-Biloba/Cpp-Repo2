#include "../param/shader.hpp"
#include <GLFW/glfw3.h>

int min(int x, int y) { return y < x ? y : x; }


static char const* julia_vert = R"~(
#version 430 core
layout (location = 0) in vec3 pos;
out vec2 coord;

void main()
{
	gl_Position = vec4(pos, 1.0);
	coord = vec2(pos.x, -pos.y);
}
)~";


static char const* julia_frag = R"~(
#version 430 core
in vec2 coord;
out vec4 FragColor;

uniform int iter;
uniform int ijulia;
uniform dvec2 org;
uniform dvec2 ppi;

void main()
{
	int i = 0;
	if (ijulia == 0)
	{
		dvec2 c = org + dvec2(coord) * ppi;
		dvec2 z = dvec2(0, 0), q;
		for (; i < iter; ++i)
		{
			q.x = z.x * z.x;
			q.y = z.y * z.y;
			if (q.x + q.y > 4.0LF)
				break;
			z.y = c.y + z.x * z.y * 2.0LF;
			z.x = c.x + q.x - q.y;
		}
	}
	else
	{
		dvec2 z = dvec2(coord) * ppi, q;
		for (; i < iter; ++i)
		{
			q.x = z.x * z.x;
			q.y = z.y * z.y;
			if (q.x + q.y > 4.0LF)
				break;
			z.y = org.y + z.x * z.y * 2.0LF;
			z.x = org.x + q.x - q.y;
		}
	}

	float s = float(i) / float(iter);
	float t, r, g, b;
	if (false)
	{
		t = 1.0 - s;
		r = clamp(t * s * s * s * 9.0,  0.0, 1.0);
		g = clamp(t * t * s * s * 15.0, 0.0, 1.0);
		b = clamp(t * t * t * s * 8.5,  0.0, 1.0);
	}
	else
	{
		t = 4.0 * s;
		r = clamp(min(t - 1.5, 4.5 - t), 0.0, 1.0);
		g = clamp(min(t - 0.5, 3.5 - t), 0.0, 1.0);
		b = clamp(min(t + 0.5, 2.5 - t), 0.0, 1.0);
	}
	gl_FragColor = vec4(r, g, b, 1.0);
}
)~";


void errorCallback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
	fflush(stderr);
}

// process all input
// query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, 1);
}


// glfw: whenever the window size changed this callback function executes
void framebufferCallback(GLFWwindow*, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	printf("window's size: %d × %d\n", width, height);
}


int main(int argc, char** argv)
{
	int ijulia = 0;
	if (argc > 1)
		ijulia = atoi(argv[1]);

	// glfw: initialize and configure
	// ------------------------------
	glfwSetErrorCallback(errorCallback);
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_SAMPLES, 2);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(600, 600, "Julia", NULL, NULL);
	if (window == NULL)
	{
		fputs("Failed to create GLFW window\n", stderr);
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferCallback);
	// glfwSwapInterval(0);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fputs("Failed to initialize GLAD\n", stderr);
		return -1;
	}
	printf("%s, %s\n",
		reinterpret_cast<char const*>(glGetString(GL_VENDOR)),
		reinterpret_cast<char const*>(glGetString(GL_RENDERER)));

	// build and compile our shader zprogram
	// ------------------------------------
	GLShader vert(GL_VERTEX_SHADER);
	GLShader frag(GL_FRAGMENT_SHADER);
	vert.loadStr(julia_vert).compile();
	frag.loadStr(julia_frag).compile();
	GLProgram prog;
	prog.attach(vert).attach(frag);
	prog.link().use();
	glUniform1i(prog.uniform("iter"), 512);
	glUniform1i(prog.uniform("ijulia"), ijulia);
	vert.release();
	frag.release();
	// glEnable(GL_MULTISAMPLE);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// positions
		+1.0F, +1.0F, 0.0F,
		+1.0F, -1.0F, 0.0F,
		-1.0F, -1.0F, 0.0F,
		-1.0F, +1.0F, 0.0F,
	};
	unsigned indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};
	unsigned VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	std::vector<GLubyte> image;
	char buffer[1024];
	int frame = 0;
	double ticksum = 0;
	double radius = 2.0, ratio = 0.98;
	double cdir = -0.001, coff = 0.0;
	// http://www.matrix67.com/blog/archives/292
	// 0.45, -0.1428; 0.285, 0.01; -0.8, 0.156; -0.70176, -0.3842
	double jucX = -0.835, jucY = -0.2321;
	int wdrows, wdcols;
	double ppi, ppiX, ppiY;
	if (!ijulia)
	{
		jucX = 0.273226261;
		jucY = 0.595153338;
	}
	if (argc > 3)
	{
		jucX = atof(argv[2]);
		jucY = atof(argv[3]);
	}

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		double tick0 = glfwGetTime();

		if (ijulia)
		{
			coff += cdir;
			if (coff < -0.5 || 0.5 < coff)
				cdir = -cdir;
		}
		else
		{
			radius *= ratio;
			if (radius <= 1e-7 || radius >= 2.0)
				ratio = 1.0 / ratio;
		}

		glfwGetWindowSize(window, &wdcols, &wdrows);
		ppi = radius / min(wdrows, wdcols);
		ppiX = ppi * wdcols;
		ppiY = ppi * wdrows;
		// input
		processInput(window);

		glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform2d(prog.uniform("ppi"), ppiX, ppiY);
		glUniform2d(prog.uniform("org"), jucX + coff, jucY);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		image.resize(wdrows * wdcols * 3);
		//glReadPixels(0, 0, wdcols, wdrows, GL_RGB, GL_UNSIGNED_BYTE, image.data());
		//savePGM(image.data(), wdcols, wdrows, frame);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();

		double tick1 = glfwGetTime();
		ticksum += tick1 - tick0;
		if (((++frame) & 31) == 0)
		{
			int i = 0;
			if (ijulia)
				i = sprintf(buffer, "juc (%+f %+f)", jucX + coff, jucY);
			else
				i = sprintf(buffer, "radius %.9f", radius);
			sprintf(buffer + i, ", frame %04d, %8.3fms, ~ %.2ffps",
				frame, 1e3 * ticksum, 32.0 / ticksum);
			puts(buffer);
			ticksum = 0;
		}
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}



