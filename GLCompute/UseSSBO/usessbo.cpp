#include "../param/shader.hpp"
#include <GLFW/glfw3.h>
using uchar = unsigned char;


int min(int x, int y) { return y < x ? y : x; }

int divup(int x, int y) { return (x + y - 1) / y; }


static char const* middlepath = R"~(G:\Sample\MidImg\)~";


static char const* source_vertex = {
	R"~(
#version 430 core

layout (location = 0) in vec2 pos;
out vec2 coord;

void main()
{
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	// [-1, 1] -> [0, 1]
	coord.x = (1.0 + pos.x) * 0.5;
	coord.y = (1.0 + pos.y) * 0.5;
}
)~" };

static char const* source_fragment = {
	R"~(
#version 430 core

in vec2 coord;
uniform int rows, cols;
layout (std430, binding=1) restrict buffer Array0
{
	int rgb[];
} image;
out vec4 fragcolor;

void main()
{
	float val = 1.0 / 255.0;
	ivec2 pos = ivec2(gl_FragCoord.xy);
	pos.x = min(pos.x, cols - 1);
	pos.y = min(pos.y, rows - 1);
	int idx = (pos.y * cols + pos.x);
	idx = image.rgb[idx];
	float R = (idx >> 16) * val;
	float G = ((idx >> 8) & 0xff) * val;
	float B = (idx & 0xff) * val;
	fragcolor = vec4(R, G, B, 1.0);
}
)~" };

static char const* source_compute = {
	R"~(
#version 430 core

uniform float ofs;
uniform int rows, cols;
layout (local_size_x = 16, local_size_y = 16) in;
layout (std430, binding=1) restrict buffer Array0
{
	int rgb[];
} image;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	pos.x = min(pos.x, cols - 1);
	pos.y = min(pos.y, rows - 1);
	int idx = (pos.y * cols + pos.x);
	float x, y, R, G, B;
	
	x = pos.x + pos.y;
	y = rows + cols;
	R = x / y + ofs;
	x = pos.x;
	y = cols;
	G = x / y - ofs * 0.7;
	x = pos.y;
	y = rows;
	B = x / y + ofs * 0.5;

	// x = gl_LocalInvocationID.x + gl_LocalInvocationID.y;
	// y = gl_WorkGroupSize.x + gl_WorkGroupSize.y;
	// R = x / y + ofs;
	// x = gl_WorkGroupID.x;
	// y = gl_NumWorkGroups.x;
	// G = x / y - ofs * 0.7;
	// x = gl_WorkGroupID.y;
	// y = gl_NumWorkGroups.y;
	// B = x / y + ofs * 0.3;
	
	R = abs(R - round(R)) * (2.0 * 255.0);
	G = abs(G - round(G)) * (2.0 * 255.0);
	B = abs(B - round(B)) * (2.0 * 255.0);	
	pos.x = (int(R) << 16) + (int(G) << 8) + int(B);
	image.rgb[idx] = pos.x;
}
)~" };


static void errorCallback(int error, const char* description)
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
	printf("\twindow size: %d × %d\n", width, height);
}


int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwSetErrorCallback(errorCallback);
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	// glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(960, 540, "SSBO Example", NULL, NULL);
	if (window == NULL)
	{
		fputs("Failed to create GLFW window\n", stderr);
		glfwTerminate();
		return -1;
	}
	// glfwHideWindow(window);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferCallback);
	// glfwSwapInterval(6);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fputs("Failed to initialize GLAD\n", stderr);
		return -1;
	}

	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		printf("OpenGL Debug Context\n");
		//glDebugMessageControl(GL_DEBUG_SOURCE_API,
			//GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0,
			GL_DEBUG_SEVERITY_MEDIUM, -1, "error message here");
	}
	GLint maxVaryingVector;
	glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVector);
	printf("%s, %s, MAX_VARYING_VECTORS %d\n",
		reinterpret_cast<char const*>(glGetString(GL_VENDOR)),
		reinterpret_cast<char const*>(glGetString(GL_RENDERER)),
		static_cast<int>(maxVaryingVector));
	glEnable(GL_FRAMEBUFFER_SRGB);

	// build and compile our shader program
	// ------------------------------------
	GLShader vert(GL_VERTEX_SHADER);
	GLShader frag(GL_FRAGMENT_SHADER);
	vert.loadStr(source_vertex).compile();
	frag.loadStr(source_fragment).compile();
	GLProgram render;
	render.attach(vert).attach(frag);
	render.link().use();
	vert.release();
	frag.release();
	GLCheckError;

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		+1.0F, +1.0F, // 右上
		+1.0F, -1.0F, // 右下
		-1.0F, +1.0F, // 左上
		-1.0F, -1.0F, // 左下
	};
	unsigned VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	GLShader comp(GL_COMPUTE_SHADER);
	GLProgram compute;
	comp.loadStr(source_compute).compile();
	compute.attach(comp);
	compute.link().use();
	comp.release();
	GLCheckError;

	int frame = 0;
	char name[256];
	int srows = 0, scols = 0;
	std::vector<uchar> image;
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
	GLCheckError;

	// prog loop
	// -----------
	double tickstart = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		double ofs = glfwGetTime() * 0.5;
		int wdrows, wdcols;
		processInput(window);
		glfwGetWindowSize(window, &wdcols, &wdrows);

		// buffer
		if ((srows != wdrows) || (scols != wdcols))
		{
			srows = wdrows;
			scols = wdcols;
			// glDeleteBuffers(1, &ssbo);
			// glBufferData 会自动删除之前的内存
			glBufferData(GL_SHADER_STORAGE_BUFFER,
				srows * scols * sizeof(int), NULL, GL_DYNAMIC_COPY);
			image.resize(srows * scols * 3);
			image[0] = 100;
			GLCheckError;
		}
		
		// compute
		compute.use();
		glUniform1i(compute.uniform("rows"), srows);
		glUniform1i(compute.uniform("cols"), scols);
		glUniform1f(compute.uniform("ofs"), static_cast<float>(ofs));
		glDispatchCompute(divup(wdcols, 16), divup(wdrows, 16), 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// render
		render.use();
		glUniform1i(render.uniform("rows"), srows);
		glUniform1i(render.uniform("cols"), scols);
		glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#if 0
		int* ssboptr = static_cast<int*>(
			glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
		for (int i = srows * scols; (--i) >= 0;)
		{
			int k = i * 3;
			int v = ssboptr[i];
			image[k + 0] = static_cast<uchar>(v >> 16);
			image[k + 1] = static_cast<uchar>(v >> 8);
			image[k + 2] = static_cast<uchar>(v);
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		snprintf(name, sizeof(name), "%s%04d.ppm", middlepath, frame);
		ppmWrite(name, image.data(), srows, scols, 3, true);
#endif

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();

		if (((++frame) & 31) == 0)
		{
			ofs = glfwGetTime() - tickstart;
			snprintf(name, sizeof(name),
				"frame %4d, %8.3fms, ~ %.2ffps",
				frame, 1e3 * ofs, 32.0 / ofs);
			puts(name);
			tickstart = glfwGetTime();
		}
	}

	glDeleteBuffers(1, &ssbo);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
