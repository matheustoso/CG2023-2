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
#include <3DViewer/animation.h>

#include <iostream>
#include <Windows.h>
#include <algorithm>
#include <string>
#include <map>
#include <shobjidl.h> 

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow* window);
void loadModel(std::string& path);
void loadScene(std::string& path);
bool openFile();
void renderLights(Shader& shader);
void renderModels(Shader& shader);

// object
struct ObjectModel {
	std::string name;
	Model model;
	bool isAnimated;
	float translateX;
	float translateY;
	float translateZ;
	bool animateRotationX;
	bool animateRotationY;
	bool animateRotationZ;
	float rotateX;
	float rotateY;
	float rotateZ;
	bool animateScale;
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
std::map<std::string, Animation> animations;
std::string selectedModel;
bool editing = false;
bool wireframe = false;

// lighting
bool spotlight = true;
glm::vec3 lightDirection = { -0.2f, -1.0f, -0.3f };
glm::vec3 lightAmbient = { 0.5f, 0.5f, 0.5f };
glm::vec3 lightDiffuse = { 0.4f, 0.4f, 0.4f };
glm::vec3 lightSpecular = { 0.5f, 0.5f, 0.5f };

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
	Shader shader("shader.vs", "shader.fs");
	shader.use();
	shader.setInt("material.diffuse", 0);
	shader.setInt("material.specular", 1);

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

		renderLights(shader);
		renderModels(shader);

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
			ImGui::SliderFloat("Translate X", &model.translateX, -100.0f, 100.0f);
			ImGui::SliderFloat("Translate Y", &model.translateY, -100.0f, 100.0f);
			ImGui::SliderFloat("Translate Z", &model.translateZ, -100.0f, 100.0f);
			ImGui::SliderFloat("Rotate X", &model.rotateX, 0.0f, 360.0f);
			ImGui::SliderFloat("Rotate Y", &model.rotateY, 0.0f, 360.0f);
			ImGui::SliderFloat("Rotate Z", &model.rotateZ, 0.0f, 360.0f);
			ImGui::SliderFloat("Scale", &model.scale, 0.001f, 10.0f);
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

void renderLights(Shader& shader) {
	shader.use();
	shader.setVec3("viewPos", camera.Position);
	shader.setFloat("material.shininess", 32.0f);

	shader.setVec3("dirLight.direction", lightDirection);
	shader.setVec3("dirLight.ambient", lightAmbient);
	shader.setVec3("dirLight.diffuse", lightDiffuse);
	shader.setVec3("dirLight.specular", lightSpecular);

	if (spotlight) {
		shader.setVec3("spotLight.position", camera.Position);
		shader.setVec3("spotLight.direction", camera.Front);
		shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
		shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		shader.setFloat("spotLight.constant", 1.0f);
		shader.setFloat("spotLight.linear", 0.09f);
		shader.setFloat("spotLight.quadratic", 0.032f);
		shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
	}
}

void renderModels(Shader& shader) {
	for (auto& x : models) {
		shader.use();

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);

		float angle = glfwGetTime();

		model = glm::translate(model, glm::vec3(x.second.translateX, x.second.translateY, x.second.translateZ));

		if (x.second.isAnimated) {
			glm::vec3 points = animations[x.first].animate();
			model = glm::translate(model, points);
		}

		model = glm::rotate(model, (float)glm::radians(x.second.rotateX), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, (float)glm::radians(x.second.rotateY), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, (float)glm::radians(x.second.rotateZ), glm::vec3(0.0f, 0.0f, 1.0f));

		if (x.second.animateRotationX) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		if (x.second.animateRotationY) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
		if (x.second.animateRotationZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

		float scaleDelta = x.second.animateScale ? (sin(angle) * (x.second.scale / 2.f)) : 0.f;
		model = glm::scale(model, glm::vec3(x.second.scale + scaleDelta, x.second.scale + scaleDelta, x.second.scale + scaleDelta));

		shader.setMat4("model", model);
		x.second.model.Draw(shader);
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
			path.substr(path.size() - 4) == "json" ? loadScene(path) : loadModel(path);
		}
	}
}

void loadModel(std::string& path) {
	Model model(path.c_str());

	Animation animation = Animation();

	ObjectModel obj;
	obj.model = model;
	obj.isAnimated = false;
	obj.translateX = 0.0f;
	obj.translateY = 0.0f;
	obj.translateZ = 0.0f;
	obj.animateRotationX = false;
	obj.animateRotationY = false;
	obj.animateRotationZ = false;
	obj.rotateX = 0.0f;
	obj.rotateY = 0.0f;
	obj.rotateZ = 0.0f;
	obj.animateScale = false;
	obj.scale = 1.0f;

	std::string name = path.substr(path.find_last_of("/") + 1, path.find_last_of(".") - path.find_last_of("/") - 1);
	int i = 0;
	while (models.count(name) > 0) {
		i++;
		name = name + std::to_string(i);
	}

	models.insert(make_pair(name, obj));
}

void loadScene(std::string& path) {
	std::ifstream data(path);
	json scene = json::parse(data);

	//camera
	camera.Position = glm::vec3(
		scene.at("camera").at("position").at("x"),
		scene.at("camera").at("position").at("y"),
		scene.at("camera").at("position").at("z"));
	camera.Front = glm::vec3(
		scene.at("camera").at("front").at("x"),
		scene.at("camera").at("front").at("y"),
		scene.at("camera").at("front").at("z"));
	camera.WorldUp = glm::vec3(
		scene.at("camera").at("worldUp").at("x"),
		scene.at("camera").at("worldUp").at("y"),
		scene.at("camera").at("worldUp").at("z"));
	camera.Yaw = scene.at("camera").at("yaw");
	camera.Pitch = scene.at("camera").at("pitch");
	camera.MovementSpeed = scene.at("camera").at("speed");

	//lighting
	spotlight = scene.at("lighting").at("spotlight");
	lightDirection = glm::vec3(
		scene.at("lighting").at("direction").at("x"),
		scene.at("lighting").at("direction").at("y"),
		scene.at("lighting").at("direction").at("z"));
	lightAmbient = glm::vec3(
		scene.at("lighting").at("ambient").at("x"),
		scene.at("lighting").at("ambient").at("y"),
		scene.at("lighting").at("ambient").at("z"));
	lightDiffuse = glm::vec3(
		scene.at("lighting").at("diffuse").at("x"),
		scene.at("lighting").at("diffuse").at("y"),
		scene.at("lighting").at("diffuse").at("z"));
	lightSpecular = glm::vec3(
		scene.at("lighting").at("specular").at("x"),
		scene.at("lighting").at("specular").at("y"),
		scene.at("lighting").at("specular").at("z"));

	//objects
	models.clear();
	models = std::map<std::string, ObjectModel>();
	int os = scene.at("objects").size();
	for (int i = 0; i < os; i++) {
		 Model model(scene.at("objects").at(i).at("path"));

		 ObjectModel obj;
		 obj.model = model;
		 obj.isAnimated = scene.at("objects").at(i).at("isAnimated");
		 obj.translateX = scene.at("objects").at(i).at("translate").at("x");
		 obj.translateY = scene.at("objects").at(i).at("translate").at("y");
		 obj.translateZ = scene.at("objects").at(i).at("translate").at("z");
		 obj.animateRotationX = scene.at("objects").at(i).at("animateRotationX");
		 obj.animateRotationY = scene.at("objects").at(i).at("animateRotationY");
		 obj.animateRotationZ = scene.at("objects").at(i).at("animateRotationZ");
		 obj.rotateX = scene.at("objects").at(i).at("rotate").at("x");
		 obj.rotateY = scene.at("objects").at(i).at("rotate").at("y");
		 obj.rotateZ = scene.at("objects").at(i).at("rotate").at("z");
		 obj.animateScale = scene.at("objects").at(i).at("animateScale");
		 obj.scale = scene.at("objects").at(i).at("scale");

		 std::string name = scene.at("objects").at(i).at("name");

		 models.insert(make_pair(name, obj));
	}

	//animations
	animations.clear();
	animations = std::map<std::string, Animation>();
	int as = scene.at("animations").size();
	for (int j = 0; j < as; j++) {
		bool loop = scene.at("animations").at(j).at("loop");
		Curves curve = static_cast<Curves>(scene.at("animations").at(j).at("curve"));
		std::string prop = scene.at("animations").at(j).at("prop");

		vector <glm::vec3> controlPoints;
		int cs = scene.at("animations").at(j).at("controlPoints").size();
		for (int c = 0; c < cs; c++) {
			glm::vec3 controlPoint = glm::vec3(
				scene.at("animations").at(j).at("controlPoints").at(c).at("x"),
				scene.at("animations").at(j).at("controlPoints").at(c).at("y"),
				scene.at("animations").at(j).at("controlPoints").at(c).at("z")
			);
			controlPoints.push_back(controlPoint);
		}

		Animation animation(loop, curve, controlPoints);

		animations.insert(make_pair(prop, animation));
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