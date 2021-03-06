#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../shader.hpp"

#include <iostream>
#include <opencv2/highgui.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(-1.0f, 0.5f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	// uncomment this statement to fix compilation on OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------
	Shader lightingShader("4.2.maps.vert", "4.2.maps.frag");
	Shader lampShader("4.2.lamp.vert", "4.2.lamp.frag");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f,
		+0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +0.0f,
		+0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +1.0f,
		+0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +1.0f, +1.0f,
		-0.5f, +0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +1.0f,
		-0.5f, -0.5f, -0.5f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f,

		-0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f,
		+0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +0.0f,
		+0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +1.0f,
		+0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +1.0f, +1.0f,
		-0.5f, +0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +1.0f,
		-0.5f, -0.5f, +0.5f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f,

		-0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f,
		-0.5f, +0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +1.0f,
		-0.5f, -0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +0.0f, +0.0f,
		-0.5f, +0.5f, +0.5f, -1.0f, +0.0f, +0.0f, +1.0f, +0.0f,

		+0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f,
		+0.5f, +0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +1.0f,
		+0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +1.0f,
		+0.5f, -0.5f, -0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +1.0f,
		+0.5f, -0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +0.0f, +0.0f,
		+0.5f, +0.5f, +0.5f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f,

		-0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f,
		+0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +1.0f,
		+0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f,
		+0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +1.0f, +0.0f,
		-0.5f, -0.5f, +0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +0.0f,
		-0.5f, -0.5f, -0.5f, +0.0f, -1.0f, +0.0f, +0.0f, +1.0f,

		-0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f,
		+0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +1.0f,
		+0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f,
		+0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +1.0f, +0.0f,
		-0.5f, +0.5f, +0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +0.0f,
		-0.5f, +0.5f, -0.5f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f
	};
	// first, configure the cube's VAO (and VBO)
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// load textures (we now use a utility function to keep the code more organized)
	// -----------------------------------------------------------------------------
	unsigned int diffuseMap =  loadTexture("../images/container2.jpg");
	unsigned int specularMap = loadTexture("../images/container2_specular.jpg");

	// shader configuration
	// --------------------
	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);


	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// be sure to activate shader when setting uniforms/drawing objects
		lightingShader.use();
		lightingShader.setVec3("light.position", lightPos);
		lightingShader.setVec3("viewPos", camera.Position);

		// light properties
		lightingShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
		lightingShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
		lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

		// material properties
		lightingShader.setFloat("material.shininess", 16.0f);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.setMat4("model", model);

		// bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		// bind specular map
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);

		// render the cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		// also draw the lamp object
		lampShader.use();
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lampShader.setMat4("model", model);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	cv::Mat m = cv::imread(path, cv::IMREAD_UNCHANGED);
	if (m.empty())
		std::cout << "Texture failed to load " << path << std::endl;
	else
	{
		GLenum from;
		int cn = m.channels();
		if (cn == 1)
			from = GL_RED;
		else if (cn == 3)
			from = GL_BGR;
		else if (cn == 4)
			from = GL_BGRA;
		else
			std::cout << "Texture channels unknown " << path << std::endl;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m.cols, m.rows, 0, from, GL_UNSIGNED_BYTE, m.data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	return textureID;
}
