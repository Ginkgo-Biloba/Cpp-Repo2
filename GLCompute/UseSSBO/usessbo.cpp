#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <atlstr.h>
#include "../param/shader.hpp"
#include <GLFW/glfw3.h>

using uchar = unsigned char;

static char const* middlepath = R"~(G:\Sample\MidImg\)~";

int divup(int x, int y) { return (x + y - 1) / y; }


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
	int ix = clamp(int(gl_FragCoord.x), 1, cols - 2);
	int iy = clamp(int(gl_FragCoord.y), 1, rows - 2);
#if 0
	float R = 0.0, G = 0.0, B = 0.0;
	for (int h = -1; h <= 1; ++h)
		for (int w = -1; w <= 1; ++w)
		{
			int val = (iy + h) * cols + (ix + w);
			val = image.rgb[val];
			R += (val >> 16);
			G += ((val >> 8) & 0xff);
			B += (val & 0xff);
		}
	float s = 1.0 / (255.0 * 9.0);
	R = R * s; G = G * s; B = B * s;
#else
	int val = image.rgb[iy * cols + ix];
	float s = 1.0 / 255.0;
	float R = s * (val >> 16);
	float G = s * ((val >> 8) & 0xff);
	float B = s * (val & 0xff);
#endif
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

int colormap(float v, float a, float b)
{
	float R = 1.0, G = 1.0, B = 1.0;
	float d = max(b - a, 1e-7);
	v = (clamp(v, a, b) - a) / d;
	if (v < 0.25) 
	{
		R = 0;
		G = 4.0 * v;
		B = 1.0 - G;
	} 
	else if (v < 0.5)
	{
		R = 0;
		B = 1 + 4.0 * (0.25 - v);
	}
	else if (v < 0.75)
	{
		R = 4.0 * (v - 0.5);
		B = 0;
	}
	else
	{
		G = 1 + 4.0 * (0.75 - v);
		B = 0;
	}
	int x = int(R * 255.0) << 16;
	int y = int(G * 255.0) << 8;
	int z = int(B * 255.0);
	return x + y + z;
}

void main()
{
	int ix = min(int(gl_GlobalInvocationID.x), cols - 1);
	int iy = min(int(gl_GlobalInvocationID.y), rows - 1);
	float x = float(cols - 1);
	float y = float(rows - 1);
	float s = (ofs + 4.0) / min(x, y);
	x = (x * 0.50 - ix) * s;
	y = (y * 0.75 - iy) * s;
	float r = length(vec2(x, y));
	float t = y / (r + 1e-7);
	float z = (1 + t - r); // <= 2.0 
	t = max(z * 0.5, 0.0); 
	t = pow(t, 0.1 + 0.09 * ofs);
	image.rgb[iy * cols + ix] = colormap(t, 0.0, 1.0);
}
)~" };


static void errorCallback(int error, const char* description)
{
	CStringA e;
	e.Format("Error %d: %s\n", error, description);
	MessageBoxA(NULL, e.GetString(), "GLFW ERROR", MB_OK);
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
	// make sure the viewport matches the new window dimensions
	// note that width and height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


#ifdef _CONSOLE
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int)
#endif
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
	CStringA title("Heart"), info;
	GLFWwindow* window = glfwCreateWindow(600, 600, title.GetString(), NULL, NULL);
	if (!window)
	{
		MessageBoxA(NULL, "failed to create GLFW window", "error", MB_OK);
		glfwTerminate();
		return 1;
	}
	// glfwHideWindow(window);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferCallback);
	// glfwSwapInterval(0);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		MessageBoxA(NULL, "failed to initialize GLAD", "error", MB_OK);
		return 2;
	}

	GLint maxVaryingVector;
	glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVector);
	title.Format("%s, %s",
		reinterpret_cast<char const*>(glGetString(GL_VENDOR)),
		reinterpret_cast<char const*>(glGetString(GL_RENDERER)));
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
	compute.link();
	comp.release();
	GLCheckError;

	int frame = 0;
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
		double cur = sin(glfwGetTime() * 3.0);
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
		glUniform1f(compute.uniform("ofs"), static_cast<float>(cur));
		glDispatchCompute(divup(wdcols, 16), divup(wdrows, 16), 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// render
		render.use();
		glUniform1i(render.uniform("rows"), srows);
		glUniform1i(render.uniform("cols"), scols);
		glClearColor(0.2F, 0.5F, 0.3F, 1.0F);
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
		// glReadPixels(0, 0, scols, srows, GL_BGR, GL_UNSIGNED_BYTE, image.data());
		info.Format("%s%04d.ppm", middlepath, frame);
		ppmWrite(info.GetString(), image.data(), srows, scols, 3, true);
#endif

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (((++frame) & 63) == 0)
		{
			cur = 64.0 / (glfwGetTime() - tickstart);
			info.Format("%s, %.2ffps", title.GetString(), cur);
			glfwSetWindowTitle(window, info.GetString());
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

