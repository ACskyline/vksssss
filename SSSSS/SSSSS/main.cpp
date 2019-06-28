#include "Renderer.h"
#include "Level.h"
#include "Scene.h"
#include "Pass.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Frame.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"//ImGui_ImplVulkan_Init and ImGui_ImplVulkan_CreateDeviceObjects are modified to support msaa

#include <iterator>

const float EPSILON = 0.001f;
const int WIDTH = 800;
const int HEIGHT = 600;
const int WIDTH_RT = 800;
const int HEIGHT_RT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;//3 in total

Renderer mRenderer(WIDTH, HEIGHT, MAX_FRAMES_IN_FLIGHT);
Level mLevel("default name");
Scene mScene("default scene");
Pass mPassOffscreen("offscreen pass");
Pass mPass("default pass");
Shader mVertShader(Shader::ShaderType::VertexShader, "standard.vert");
Shader mFragShader(Shader::ShaderType::FragmentShader, "standard.frag");
Shader mDeferredVertShader(Shader::ShaderType::VertexShader, "deferred.vert");
Shader mDeferredFragShader(Shader::ShaderType::FragmentShader, "deferred.frag");
OrbitCamera mCameraOffscreen("offscreen camera", 3, 45.f, 45.f, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_RT, HEIGHT_RT, 0.1f, 10.f);
Camera mCamera("default camera", glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH, HEIGHT, 0.1f, 10.f);
OrbitCamera* pCurrentOrbitCamera = &mCameraOffscreen;
Mesh mMeshHead("smooth_head.obj", Mesh::MeshType::File, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Mesh mMeshFullScreenQuad("quad", Mesh::MeshType::FullScreenQuad, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
//Mesh mMeshSquare("square", Mesh::MeshType::Square, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(2, 2, 1));
Texture mTextureColor("head_color.jpg", VK_FORMAT_R8G8B8A8_UNORM);
Texture mTextureNormal("head_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
RenderTexture mRenderTexture("offscreen rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, true);

//imgui stuff
VkDescriptorPool ImGuiDescriptorPool;
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

//io stuff
bool MOUSE_LEFT_BUTTON_DOWN = false;
bool MOUSE_RIGHT_BUTTON_DOWN = false;
bool MOUSE_MIDDLE_BUTTON_DOWN = false;

void CreatePasses()
{
	mPassOffscreen.SetCamera(&mCameraOffscreen);
	mPassOffscreen.AddMesh(&mMeshHead);
	//mPassOffscreen.AddMesh(&mMeshSquare);
	mPassOffscreen.AddTexture(&mTextureColor);
	mPassOffscreen.AddTexture(&mTextureNormal);
	mPassOffscreen.AddRenderTexture(&mRenderTexture);
	mPassOffscreen.AddShader(&mVertShader);
	mPassOffscreen.AddShader(&mFragShader);

	mPass.SetCamera(&mCamera);
	mPass.AddMesh(&mMeshFullScreenQuad);
	mPass.AddTexture(&mRenderTexture);
	mPass.AddShader(&mDeferredVertShader);
	mPass.AddShader(&mDeferredFragShader);
}

void CreateScenes()
{
	mScene.AddPass(&mPassOffscreen);
	mScene.AddPass(&mPass);
}

void CreateLevels()
{
	mLevel.AddCamera(&mCamera);
	mLevel.AddCamera(&mCameraOffscreen);
	mLevel.AddMesh(&mMeshHead);
	mLevel.AddMesh(&mMeshFullScreenQuad);
	//mLevel.AddMesh(&mMeshSquare);
	mLevel.AddPass(&mPass);
	mLevel.AddPass(&mPassOffscreen);
	mLevel.AddScene(&mScene);
	mLevel.AddShader(&mVertShader);
	mLevel.AddShader(&mFragShader);
	mLevel.AddShader(&mDeferredVertShader);
	mLevel.AddShader(&mDeferredFragShader);
	mLevel.AddTexture(&mTextureColor);
	mLevel.AddTexture(&mTextureNormal);
	mLevel.AddTexture(&mRenderTexture);
}

void MouseButton(GLFWwindow* window, int button, int action, int mods)
{
	//this function is not used
	//printf("button:%d,action:%d,mods:%d\n", button, action, mods);
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		MOUSE_LEFT_BUTTON_DOWN = static_cast<bool>(action);
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		MOUSE_RIGHT_BUTTON_DOWN = static_cast<bool>(action);
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		MOUSE_MIDDLE_BUTTON_DOWN = static_cast<bool>(action);
		break;
	default:
		break;
	}
}

void MouseMotion(GLFWwindow* window, double x, double y)
{
	const float horizontalFactor = -0.1f;
	const float verticalFactor = 0.1f;
	const float horizontalPanFactor = -0.001f;
	const float verticalPanFactor = 0.001f;
	float static xOld = static_cast<float>(x);
	float static yOld = static_cast<float>(y);
	float xDelta = static_cast<float>(x) - xOld;
	float yDelta = static_cast<float>(y) - yOld;
	bool updateCamera = false;
	int keyC = glfwGetKey(window, GLFW_KEY_C);
	if (MOUSE_LEFT_BUTTON_DOWN && keyC == GLFW_PRESS)
	{
		if (glm::abs(xDelta) > EPSILON)
		{
			pCurrentOrbitCamera->horizontalAngle += xDelta * horizontalFactor;
			updateCamera = true;
		}
		if (glm::abs(yDelta) > EPSILON)
		{
			pCurrentOrbitCamera->verticalAngle += yDelta * verticalFactor;
			if (pCurrentOrbitCamera->verticalAngle > 90 - EPSILON) pCurrentOrbitCamera->verticalAngle = 89 - EPSILON;
			if (pCurrentOrbitCamera->verticalAngle < -90 + EPSILON) pCurrentOrbitCamera->verticalAngle = -89 + EPSILON;
			updateCamera = true;
		}
	}

	if (MOUSE_MIDDLE_BUTTON_DOWN && keyC == GLFW_PRESS)
	{
		if (glm::abs(xDelta) > EPSILON || glm::abs(yDelta) > EPSILON)
		{
			glm::vec3 forward = glm::normalize(pCurrentOrbitCamera->target - pCurrentOrbitCamera->position);
			glm::vec3 up = pCurrentOrbitCamera->up;
			glm::vec3 right = glm::cross(forward, up);
			up = glm::cross(right, forward);

			if (glm::abs(xDelta) > EPSILON)
			{
				pCurrentOrbitCamera->target += horizontalPanFactor * xDelta * right;
			}

			if (glm::abs(yDelta) > EPSILON)
			{
				pCurrentOrbitCamera->target += verticalPanFactor * yDelta * up;
			}
			updateCamera = true;
		}
	}

	if (updateCamera)
	{
		pCurrentOrbitCamera->UpdatePosition();
		mPassOffscreen.UpdatePassUniformBuffer(&mCameraOffscreen);
	}

	xOld = static_cast<float>(x);
	yOld = static_cast<float>(y);
}

void MouseScroll(GLFWwindow* window, double x, double y)
{
	const float zFactor = 0.1f;
	const float& direction = static_cast<float>(y);

	bool updateCamera = false;
	int keyC = glfwGetKey(window, GLFW_KEY_C);
	if (keyC == GLFW_PRESS)
	{
		if (glm::abs(direction - EPSILON) > 0)
		{
			pCurrentOrbitCamera->distance -= direction * zFactor;
			if (pCurrentOrbitCamera->distance < 0 + EPSILON) pCurrentOrbitCamera->distance = 0.1f + EPSILON;
			updateCamera = true;
		}
	}

	if (updateCamera)
	{
		pCurrentOrbitCamera->UpdatePosition();
		mPassOffscreen.UpdatePassUniformBuffer(&mCameraOffscreen);
	}
}

void ResizeCallback(GLFWwindow* window, int width, int height) 
{
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	if (width != mRenderer.width || height != mRenderer.height)
		throw std::runtime_error("window size does not match original!");
}

void CreateWindow() 
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//disable resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mRenderer.window = glfwCreateWindow(WIDTH, HEIGHT, "Yes, Vulkan!", nullptr, nullptr);

	//handle resize explicitly        
	glfwSetFramebufferSizeCallback(mRenderer.window, ResizeCallback);

	glfwSetMouseButtonCallback(mRenderer.window, MouseButton);
	glfwSetCursorPosCallback(mRenderer.window, MouseMotion);
	glfwSetScrollCallback(mRenderer.window, MouseScroll);
}

void InitRenderer()
{
	//1.add levels to the renderer
	mRenderer.AddLevel(&mLevel);

	//2.initialize vulkan
	mRenderer.InitVulkan();

	//TODO: add textures in to mRenderer.frameVec if necessary AFTER InitVulkan, because this depends on the number of swap images
	//for (auto& frame : mRenderer.frameVec)
	//{
	//	frame.AddTexture(...);
	//}

	//3.parse levels to create descriptor pool and initialize levels
	mRenderer.InitAssets();

	//4.pipelines
	mRenderer.CreatePipeline(
		mRenderer.offscreenPipeline,
		mRenderer.offscreenPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPassOffscreen);

	mRenderer.CreatePipeline(
		mRenderer.graphicsPipeline,
		mRenderer.graphicsPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPass);
}

void InitImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	//it seems imgui only require one combined image sampler descriptor
	mRenderer.CreateDescriptorPool(ImGuiDescriptorPool, 1, 0, 1);

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(mRenderer.GetWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = mRenderer.GetInstance();
	init_info.PhysicalDevice = mRenderer.GetPhysicalDevice();
	init_info.Device = mRenderer.GetDevice();
	init_info.QueueFamily = mRenderer.GetGraphicsQueueFamilyIndex();
	init_info.Queue = mRenderer.GetGraphicsQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = ImGuiDescriptorPool;
	init_info.Allocator = VK_NULL_HANDLE;
	init_info.CheckVkResultFn = VK_NULL_HANDLE;
	ImGui_ImplVulkan_Init(&init_info, mRenderer.swapChainRenderPass, mRenderer.GetSwapChainMsaaSamples());

	// Setup Style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Upload Fonts
	{
		// Use any command queue
		VkCommandPool command_pool = mRenderer.defaultCommandPool;
		VkCommandBuffer command_buffer = mRenderer.defaultCommandBuffers[0];

		VkResult err = vkResetCommandPool(mRenderer.GetDevice(), command_pool, 0);
		if (err != VK_SUCCESS)
			throw std::runtime_error("failed to upload fonts!");

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(command_buffer, &begin_info);
		if (err != VK_SUCCESS)
			throw std::runtime_error("failed to upload fonts!");

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &command_buffer;
		err = vkEndCommandBuffer(command_buffer);
		if (err != VK_SUCCESS)
			throw std::runtime_error("failed to upload fonts!");
		err = vkQueueSubmit(mRenderer.GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
		if (err != VK_SUCCESS)
			throw std::runtime_error("failed to upload fonts!");

		err = vkDeviceWaitIdle(mRenderer.GetDevice()); 
		if (err != VK_SUCCESS)
			throw std::runtime_error("failed to upload fonts!");
		ImGui_ImplVulkan_InvalidateFontUploadObjects();
	}
}

void DrawImGui(VkCommandBuffer commandBuffer)
{
	static int mode = 0;
	static int counter = 0;
	bool updateScene = false;

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


	ImGui::Begin("SSSSS");

	ImGui::Text("This is some useful text.");

	ImGui::SliderInt("mode", &mode, 0, 4);
	if (mode != mScene.sUBO.mode)
	{
		mScene.sUBO.mode = mode;
		updateScene = true;
	}

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	if (updateScene)
		mScene.UpdateSceneUniformBuffer();

	// Rendering
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Draw()
{
	uint32_t imageIndex = mRenderer.WaitForFence();
	mRenderer.BeginCommandBuffer(mRenderer.defaultCommandBuffers[imageIndex]);
	//mRenderer.frameVec[imageIndex].UpdateFrameUniformBuffer();

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.graphicsPipelineLayout,
		static_cast<int>(UNIFORM_SLOT::FRAME),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.graphicsPipelineLayout,
		static_cast<int>(UNIFORM_SLOT::SCENE),
		1,
		mScene.GetSceneDescriptorSetPtr(),
		0,
		nullptr);

	//1st pass

	mRenderTexture.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);

	mRenderer.RecordCommand(
		glm::vec4(0.8, 0.7, 0.8, 1.0),
		glm::vec2(1.0, 0),
		mPassOffscreen,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.offscreenPipeline,
		mRenderer.offscreenPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent);

	//2nd pass

	mRenderTexture.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);

	mRenderer.RecordCommandNoEnd(
		glm::vec4(0.3, 0.6, 1.0, 1.0),
		glm::vec2(1.0, 0),
		mPass,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.graphicsPipeline,
		mRenderer.graphicsPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent);

	//render pass remains the same which is swap chain render pass,
	//so this has to be the last render command (which renders to a swap chain framebuffer)
	DrawImGui(mRenderer.defaultCommandBuffers[imageIndex]);

	//for the stupid render pass mechanism
	mRenderer.RecordCommandEnd(mRenderer.defaultCommandBuffers[imageIndex]);

	mRenderer.EndCommandBuffer(mRenderer.defaultCommandBuffers[imageIndex], imageIndex);
}

void MainLoop()
{
	//main loop
	while (!glfwWindowShouldClose(mRenderer.window))
	{
		glfwPollEvents();
		Draw();
	}
}

void CleanUp()
{
	mRenderer.IdleWait();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(mRenderer.GetDevice(), ImGuiDescriptorPool, nullptr);
	mRenderer.CleanUp();
	glfwDestroyWindow(mRenderer.window);
	glfwTerminate();
}

int main() 
{
	try 
	{
		CreatePasses();
		CreateScenes();
		CreateLevels();
		CreateWindow();
		InitRenderer();
		InitImGui();
		MainLoop();
		CleanUp();
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	getchar();
	return EXIT_SUCCESS;
}
