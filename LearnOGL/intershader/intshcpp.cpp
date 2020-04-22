#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>

#pragma comment(lib, "glfw3.lib")

char const* vertexSS = "#version 430 core\n"
"layout (location = 0) in vec3 pos;\n"  // 位置变量的属性位置值为 0 
"layout (location = 1) in vec3 vcol;\n" // 颜色变量的属性位置值为 1
"out vec3 acol;\n" // 向片段着色器输出一个颜色
"void main()\n"
"{\n" 
"  gl_Position = vec4(pos.x, pos.y, pos.z, 1.F);\n"
"  acol = vcol;\n"
"}\n"
"\0\n";

char const* fragSS = "#version 430 core\n"
"out vec4 color;\n"
"in vec3 acol;"
"void main()\n"
"{ color = vec4(acol.x, acol.y, acol.z, 1.F); }\n"
"\0\n";


// process all input
// query GLFW whether relevant keys are pressed/released this frame and react accordingly
void frame_size_change(GLFWwindow*, int w, int h)
{
	// make sure the viewport matches the new window dimensions;
	// note that width and height will be significantly larger than specified on retina displays.
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
	GLFWwindow* wd = glfwCreateWindow(800, 600, "Interpolated shader", NULL, NULL);
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

	//===== build and compile our shader program =====//

	// 顶点着色器
	unsigned vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSS, NULL);
	glCompileShader(vertexShader);
	// 检查编译错误
	int status;
	char info[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		glGetShaderInfoLog(vertexShader, sizeof(info), NULL, info);
		printf("vertexShader: %s\n", info);
	}

	// 片段着色器
	unsigned fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragSS, NULL);
	glCompileShader(fragShader);
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		glGetShaderInfoLog(fragShader, sizeof(info), NULL, info);
		printf("fragShader: %s\n", info);
	}

	// 链接着色器
	unsigned shaderProg = glCreateProgram();
	glAttachShader(shaderProg, vertexShader);
	glAttachShader(shaderProg, fragShader);
	glLinkProgram(shaderProg);
	glGetProgramiv(shaderProg, GL_LINK_STATUS, &status);
	if (!status)
	{
		glGetProgramInfoLog(shaderProg, sizeof(info), NULL, info);
		printf("shaderProg: %s\n", info);
	}

	// 删除着色器
	glDeleteShader(vertexShader);
	glDeleteShader(fragShader);

	// 设置定点数据，配置顶点属性
	float const vertices[] =
	{
		// 坐标              颜色
		+0.5F, -0.5F, +0.0F, 1.0F, 0.0F, 0.0F, // 右下，红色
		-0.5F, -0.5F, +0.0F, 0.0F, 1.0F, 0.0F, // 左下，绿色
		+0.0F, +0.5F, +0.0F, 0.0F, 0.0F, 1.0F,// 正上，蓝色
	};

	// 顶点缓冲对象 (Vertex Buffer Objects, VBO)
	unsigned vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	// 绑定定点数组，然后绑定定点缓冲，再配置定点属性
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	// 位置属性
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	void* const offset1 = reinterpret_cast<void*>(3 * sizeof(float));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), offset1);
	glEnableVertexAttribArray(1);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO but this rarely happens. 
	// Modifying other VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	// glBindVertexArray(0);

	// bind the VAO (it was already bound, but just to demonstrate)
	// seeing as we only have a single VAO we can 
	// just bind it beforehand before rendering the respective triangle; this is another approach.
	glBindVertexArray(vao);

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	while (!glfwWindowShouldClose(wd))
	{
		// input
		proc_input(wd);

		// render
		glClearColor(0.2F, 0.3F, 0.3F, 1.F);
		glClear(GL_COLOR_BUFFER_BIT);

		// 激活着色器程序
		glUseProgram(shaderProg);
		// seeing as we only have a single VAO there's no need to bind it every time, 
		// but we'll do so to keep things a bit more organized
		// glBindVertexArray(vao);

		// 画三角形
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// no need to unbind it every time 
		// glBindVertexArray(0);

		// glfw: swap buffers and poll IO events 
		// (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(wd);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	// glfw: terminate, clearing all previously allocated GLFW resources
	glfwTerminate();

	return 0;
}

