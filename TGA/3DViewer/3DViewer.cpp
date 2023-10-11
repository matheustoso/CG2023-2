#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3DViewer/shader.h>
#include <3DViewer/camera.h>
#include <3DViewer/model.h>

#include <iostream>
#include <Windows.h>
#include <algorithm>
#include <string>
#include <map>
#include <shobjidl.h> 

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window);
bool openFile();
void renderModels();

// object
struct ObjectModel {
	Shader shader;
	Model model;
	std::string name;
	float translateX;
	float translateY;
	float translateZ;
	float rotateX;
	float rotateY;
	float rotateZ;
	float scale;
};

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cameraEnabled = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// input
bool keys[1024];
bool keysProcessed[1024];
std::string sSelectedFile;
std::string sFilePath;

// models
std::map<std::string, ObjectModel> models;
std::string selectedModel;
bool editing = false;
bool wireframe = false;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3DViewer", NULL, NULL);
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
	glfwSetKeyCallback(window, key_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	stbi_set_flip_vertically_on_load(true);

	glEnable(GL_DEPTH_TEST);

	//IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Setup Imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		renderModels();

		//IMGUI
		ImGui::Begin("Controls");
		ImGui::Text("P - Load object file");
		ImGui::Text("F - Toggle wireframe");
		ImGui::Text("SPACE - Toggle camera");
		ImGui::Text("WASD - Move");
		ImGui::Text("MOUSE - Look");
		ImGui::Text("SCROLL - Zoom");
		ImGui::Text("LEFT BRACKET - Reduce camera speed");
		ImGui::Text("RIGHT BRACKET - Increase camera speed");
		ImGui::End();

		ImGui::Begin("Camera Speed");
		ImGui::Text(std::to_string(camera.MovementSpeed).c_str());
		ImGui::End();

		ImGui::Begin("Objects");
		for (std::map<std::string, ObjectModel>::iterator it = models.begin(); it != models.end(); ++it) {
			if (ImGui::Button(it->first.c_str())) {
				if (selectedModel == it->first) {
					selectedModel.clear();
					editing = false;
				}
				else {
					selectedModel = it->first;
					editing = true;
				}
			}
		}
		ImGui::End();

		if (editing) {
			ObjectModel& model = models.at(selectedModel);

			ImGui::Begin(selectedModel.c_str());
			ImGui::SliderFloat("Translate X", &model.translateX, -10.0f, 10.0f);
			ImGui::SliderFloat("Translate Y", &model.translateY, -10.0f, 10.0f);
			ImGui::SliderFloat("Translate Z", &model.translateZ, -10.0f, 10.0f);
			ImGui::SliderFloat("Rotate X", &model.rotateX, 0.0f, 360.0f);
			ImGui::SliderFloat("Rotate Y", &model.rotateY, 0.0f, 360.0f);
			ImGui::SliderFloat("Rotate Z", &model.rotateZ, 0.0f, 360.0f);
			ImGui::SliderFloat("Scale", &model.scale, 0.1f, 5.0f);
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//Shutdown imgui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void renderModels() {
	for (auto& x : models) {
		x.second.shader.use();

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		x.second.shader.setMat4("projection", projection);
		x.second.shader.setMat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, glm::vec3(x.second.translateX, x.second.translateY, x.second.translateZ));

		model = glm::rotate(model, (float)glm::radians(x.second.rotateX), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, (float)glm::radians(x.second.rotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, (float)glm::radians(x.second.rotateZ), glm::vec3(0.0f, 0.0f, 1.0f));

		model = glm::scale(model, glm::vec3(x.second.scale, x.second.scale, x.second.scale));

		x.second.shader.setMat4("model", model);
		x.second.model.Draw(x.second.shader);
	}
}

void processInput(GLFWwindow* window)
{
	if (cameraEnabled) {
		if (keys[GLFW_KEY_W])
			camera.ProcessKeyboard(FORWARD, deltaTime);
		if (keys[GLFW_KEY_S])
			camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (keys[GLFW_KEY_A])
			camera.ProcessKeyboard(LEFT, deltaTime);
		if (keys[GLFW_KEY_D])
			camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	if (keys[GLFW_KEY_LEFT_BRACKET] and !keysProcessed[GLFW_KEY_LEFT_BRACKET]) {
		keysProcessed[GLFW_KEY_LEFT_BRACKET] = true;
		camera.MovementSpeed -= 1.0f;
	}

	if (keys[GLFW_KEY_RIGHT_BRACKET] and !keysProcessed[GLFW_KEY_RIGHT_BRACKET]) {
		keysProcessed[GLFW_KEY_RIGHT_BRACKET] = true;
		camera.MovementSpeed += 1.0f;
	}

	if (keys[GLFW_KEY_SPACE] and !keysProcessed[GLFW_KEY_SPACE]) {
		keysProcessed[GLFW_KEY_SPACE] = true;
		cameraEnabled ? glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL) : glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cameraEnabled = !cameraEnabled;
	}

	if (keys[GLFW_KEY_F] and !keysProcessed[GLFW_KEY_F]) {
		keysProcessed[GLFW_KEY_F] = true;
		wireframe ? glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) : glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		wireframe = !wireframe;
	}

	if (keys[GLFW_KEY_P] and !keysProcessed[GLFW_KEY_P]) {
		keysProcessed[GLFW_KEY_P] = true;
		if (openFile()) {

			std::replace(sFilePath.begin(), sFilePath.end(), '\\', '/');
			std::string path = sFilePath.substr(sFilePath.find("resources"), sFilePath.size());
			Model model(path.c_str());

			ObjectModel obj;
			obj.model = model;
			obj.shader = Shader("shader.vs", "shader.fs");
			obj.translateX = 0.0f;
			obj.translateY = 0.0f;
			obj.translateZ = 0.0f;
			obj.rotateX = 0.0f;
			obj.rotateY = 0.0f;
			obj.rotateZ = 0.0f;
			obj.scale = 1.0f;

			std::string name = path.substr(path.find_last_of("/") + 1, path.find_last_of(".") - path.find_last_of("/") - 1);
			int i = 0;
			while (models.count(name) > 0) {
				i++;
				name = name + std::to_string(i);
			}

			models.insert(make_pair(name, obj));
		}
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	if (cameraEnabled)
		camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (cameraEnabled)
		camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
			keysProcessed[key] = false;
		}
	}
}

bool openFile()
{
	HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(f_SysHr))
		return FALSE;

	IFileOpenDialog* f_FileSystem;
	f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
	if (FAILED(f_SysHr)) {
		CoUninitialize();
		return FALSE;
	}

	f_SysHr = f_FileSystem->Show(NULL);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	IShellItem* f_Files;
	f_SysHr = f_FileSystem->GetResult(&f_Files);
	if (FAILED(f_SysHr)) {
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	PWSTR f_Path;
	f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
	if (FAILED(f_SysHr)) {
		f_Files->Release();
		f_FileSystem->Release();
		CoUninitialize();
		return FALSE;
	}

	std::wstring path(f_Path);
	std::string c(path.begin(), path.end());
	sFilePath = c;

	const size_t slash = sFilePath.find_last_of("/\\");
	sSelectedFile = sFilePath.substr(slash + 1);

	CoTaskMemFree(f_Path);
	f_Files->Release();
	f_FileSystem->Release();
	CoUninitialize();
	return TRUE;
}