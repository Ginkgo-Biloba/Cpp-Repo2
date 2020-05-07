#include "../param/shader.hpp"
#include <GLFW/glfw3.h>

int min(int x, int y) { return y < x ? y : x; }

int divup(int x, int y) { return (x + y - 1) / y; }


static char const* julia_vert = R"~(
#version 430 core
layout (location = 0) in vec2 pos;
out vec2 coord;

void main()
{
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	// [-1, 1] -> [0, 1]
	coord.x = (1.0 + pos.x) * 0.5;
	coord.y = (1.0 - pos.y) * 0.5;
}
)~";

static char const* julia_frag = R"~(
#version 430 core
layout (binding = 0) uniform sampler2D siter;
in vec2 coord;

void main()
{
	float s = texture2D(siter, coord).x;
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



static char const* julia_comp = R"~(
#version 430 core

uniform int iter;
uniform int ijulia;
uniform double ppi;
uniform dvec2 org;
uniform dvec2 hwd;

// In order to write to a texture, we have to introduce it as image2D.
// local_size_x/y/z layout variables define the work group size.
// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
// gl_LocalInvocationID is the local index within the work group, and
// gl_WorkGroupID is the work group's index
layout (local_size_x = 32, local_size_y = 32) in;
layout (r32f, binding = 0) writeonly uniform image2D siter;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	dvec2 crd = dvec2(pos.x - hwd.x, pos.y - hwd.y);
	
	int i = 0;
	if (ijulia == 0)
	{
		dvec2 c = org + crd * ppi;
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
		dvec2 z = crd * ppi, q;
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
	imageStore(siter, pos, vec4(s));
}
)~";


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
	// glfwWindowHint(GLFW_SAMPLES, 4);

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
	vert.release();
	frag.release();
	// glEnable(GL_MULTISAMPLE);

	GLShader comp(GL_COMPUTE_SHADER);
	GLProgram calc;
	comp.loadStr(julia_comp).compile();
	calc.attach(comp);
	calc.link().use();
	glUniform1i(calc.uniform("iter"), 512);
	glUniform1i(calc.uniform("ijulia"), ijulia);
	comp.release();

	// Texture
	int srows = 0, scols = 0;
	unsigned siter;
	glGenTextures(1, &siter);
	glActiveTexture(GL_TEXTURE0);
	std::vector<GLubyte> image;

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
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

	char buffer[1024];
	int frame = 0;
	double ticksum = 0;
	double radius = 1.5, ratio = 0.98;
	double cdir = -0.001, coff = 0.0;
	// http://www.matrix67.com/blog/archives/292
	// 0.45, -0.1428; 0.285, 0.01; -0.8, 0.156; -0.70176, -0.3842
	double jucX = -0.835, jucY = -0.2321, ppi;
	int wdrows, wdcols;
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

	// prog loop
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
		// input
		processInput(window);

		// texture
		if ((srows != wdrows) || (scols != wdcols))
		{
			srows = wdrows; scols = wdcols;
			printf("new texture: %d × %d\n", scols, srows);
			glDeleteTextures(1, &siter);
			glGenTextures(1, &siter);
			glBindTexture(GL_TEXTURE_2D, siter);
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, scols, srows);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			// Because we're also using this tex as an image (in order to write to it),
			// we bind it to an image unit as well
			glBindImageTexture(0, siter, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
			image.resize(srows * scols);
		}

		// compute
		calc.use();
		glUniform1d(calc.uniform("ppi"), ppi);
		glUniform2d(calc.uniform("org"), jucX + coff, jucY);
		glUniform2d(calc.uniform("hwd"), wdcols * 0.5, wdrows * 0.5);
		glDispatchCompute(divup(wdcols, 32), divup(wdcols, 32), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		// glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, image.data());

		// render
		prog.use();
		glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
