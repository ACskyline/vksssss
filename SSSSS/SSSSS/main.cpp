#include "Renderer.h"
#include "Level.h"
#include "Scene.h"
#include "Pass.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Frame.h"
#include "Light.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"//ImGui_ImplVulkan_Init and ImGui_ImplVulkan_CreateDeviceObjects are modified to support msaa

#include <iterator>

const float EPSILON = 0.001f;
const int WIDTH = 800;
const int HEIGHT = 600;
const int WIDTH_RT = 800;
const int HEIGHT_RT = 600;
const int WIDTH_SHADOW_MAP = 800;
const int HEIGHT_SHADOW_MAP = 800;
const int MAX_FRAMES_IN_FLIGHT = 2;//3 in total

Renderer mRenderer(WIDTH, HEIGHT, MAX_FRAMES_IN_FLIGHT);
Level mLevel("default level");
Scene mScene("default scene");
Pass mPassDeferred("deferred pass", true);
Pass mPassSkin("skin pass", true);
Pass mPassStandard("standard pass", false);
Pass mPassShadowRed("shadow pass red", true);
Pass mPassShadowGreen("shadow pass green", true);
Pass mPassShadowBlue("shadow pass blue", true);
Pass mPassShadowBlurToH("shadow pass blur to H", true);//intro
Pass mPassShadowBlurFromHToV("shadow pass blur from H to V", true);//repeat 1
Pass mPassShadowBlurFromVToH("shadow pass blur from V to H", true);//repeat 2
Pass mPassShadowBlurFromV("shadow pass blur from V", true);//outro
Shader mVertShaderSkin(Shader::ShaderType::VertexShader, "skin.vert");
Shader mFragShaderSkin(Shader::ShaderType::FragmentShader, "skin.frag");
Shader mVertShaderStandard(Shader::ShaderType::VertexShader, "standard.vert");
Shader mFragShaderStandard(Shader::ShaderType::FragmentShader, "standard.frag");
Shader mVertShaderDeferred(Shader::ShaderType::VertexShader, "deferred.vert");
Shader mFragShaderDeferred(Shader::ShaderType::FragmentShader, "deferred.frag");
Shader mVertShaderShadow(Shader::ShaderType::VertexShader, "shadow.vert");
Shader mFragShaderShadow(Shader::ShaderType::FragmentShader, "shadow.frag");
Shader mFragShaderBlurH(Shader::ShaderType::FragmentShader, "blurh.frag");//new
Shader mFragShaderBlurV(Shader::ShaderType::FragmentShader, "blurv.frag");//new
Camera mCameraDeferred("deferred camera", glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH, HEIGHT, 0.1f, 50.f);
Camera mCameraRedLight("red light camera", glm::vec3(3, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
Camera mCameraGreenLight("green light camera", glm::vec3(0, 3, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
Camera mCameraBlueLight("blue light camera", glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
OrbitCamera mCameraOffscreen("offscreen camera", 3, 45.f, 45.f, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_RT, HEIGHT_RT, 0.1f, 50.f);
OrbitCamera* pCurrentOrbitCamera = &mCameraOffscreen;
Mesh mMeshHead("ball.obj", Mesh::MeshType::File, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Mesh mMeshSquareX("square x", Mesh::MeshType::Square, glm::vec3(-3, 0, 0), glm::vec3(0, 90, 0), glm::vec3(6, 6, 1));
Mesh mMeshSquareY("square y", Mesh::MeshType::Square, glm::vec3(0, -3, 0), glm::vec3(-90, 0, 0), glm::vec3(6, 6, 1));
Mesh mMeshSquareZ("square z", Mesh::MeshType::Square, glm::vec3(0, 0, -3), glm::vec3(0, 0, 0), glm::vec3(6, 6, 1));
Mesh mMeshFullScreenQuad("quad", Mesh::MeshType::FullScreenQuad, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Texture mTextureColorSkin("head_color.jpg", VK_FORMAT_R8G8B8A8_UNORM);
Texture mTextureNormalSkin("head_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
Texture mTextureColorBrick("brick_color.png", VK_FORMAT_R8G8B8A8_UNORM);
Texture mTextureNormalBrick("brick_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
RenderTexture mRenderTextureDiffuse("diffuse rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, true);
RenderTexture mRenderTextureSpecular("specular rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, true);
RenderTexture mRenderTextureRedLight("red light rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);
RenderTexture mRenderTextureGreenLight("green light rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);
RenderTexture mRenderTextureBlueLight("blue light rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);
RenderTexture mRenderTextureBlurH("blur h rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);//new
RenderTexture mRenderTextureBlurV("blur v rt", WIDTH_RT, HEIGHT_RT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);//new
Light mLightRed("light red", glm::vec3(1, 0, 0), glm::vec3(3, 0, 0), &mCameraRedLight, &mRenderTextureRedLight);
Light mLightGreen("light green", glm::vec3(0, 1, 0), glm::vec3(0, 3, 0), &mCameraGreenLight, &mRenderTextureGreenLight);
Light mLightBlue("light blue", glm::vec3(0, 0, 1), glm::vec3(0, 0, 3), &mCameraBlueLight, &mRenderTextureBlueLight);

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
	mPassSkin.SetCamera(&mCameraOffscreen);
	mPassSkin.AddMesh(&mMeshHead);
	mPassSkin.AddTexture(&mTextureColorSkin);
	mPassSkin.AddTexture(&mTextureNormalSkin);
	mPassSkin.AddRenderTexture(&mRenderTextureDiffuse);
	mPassSkin.AddRenderTexture(&mRenderTextureSpecular);
	mPassSkin.AddShader(&mVertShaderSkin);
	mPassSkin.AddShader(&mFragShaderSkin);

	mPassStandard.SetCamera(&mCameraOffscreen);
	mPassStandard.AddMesh(&mMeshSquareX);
	mPassStandard.AddMesh(&mMeshSquareY);
	mPassStandard.AddMesh(&mMeshSquareZ);
	mPassStandard.AddTexture(&mTextureColorBrick);
	mPassStandard.AddTexture(&mTextureNormalBrick);
	mPassStandard.AddRenderTexture(&mRenderTextureDiffuse);
	mPassStandard.AddRenderTexture(&mRenderTextureSpecular);
	mPassStandard.AddShader(&mVertShaderStandard);
	mPassStandard.AddShader(&mFragShaderStandard);

	mPassDeferred.SetCamera(&mCameraDeferred);
	mPassDeferred.AddMesh(&mMeshFullScreenQuad);
	mPassDeferred.AddTexture(&mRenderTextureDiffuse);
	mPassDeferred.AddTexture(&mRenderTextureSpecular);
	mPassDeferred.AddShader(&mVertShaderDeferred);
	mPassDeferred.AddShader(&mFragShaderDeferred);

	mPassShadowRed.SetCamera(&mCameraRedLight);
	mPassShadowRed.AddMesh(&mMeshHead);
	mPassShadowRed.AddMesh(&mMeshSquareX);
	mPassShadowRed.AddMesh(&mMeshSquareY);
	mPassShadowRed.AddMesh(&mMeshSquareZ);
	mPassShadowRed.AddRenderTexture(&mRenderTextureRedLight);
	mPassShadowRed.AddShader(&mVertShaderShadow);
	mPassShadowRed.AddShader(&mFragShaderShadow);

	mPassShadowGreen.SetCamera(&mCameraGreenLight);
	mPassShadowGreen.AddMesh(&mMeshHead);
	mPassShadowGreen.AddMesh(&mMeshSquareX);
	mPassShadowGreen.AddMesh(&mMeshSquareY);
	mPassShadowGreen.AddMesh(&mMeshSquareZ);
	mPassShadowGreen.AddRenderTexture(&mRenderTextureGreenLight);
	mPassShadowGreen.AddShader(&mVertShaderShadow);
	mPassShadowGreen.AddShader(&mFragShaderShadow);

	mPassShadowBlue.SetCamera(&mCameraBlueLight);
	mPassShadowBlue.AddMesh(&mMeshHead);
	mPassShadowBlue.AddMesh(&mMeshSquareX);
	mPassShadowBlue.AddMesh(&mMeshSquareY);
	mPassShadowBlue.AddMesh(&mMeshSquareZ);
	mPassShadowBlue.AddRenderTexture(&mRenderTextureBlueLight);
	mPassShadowBlue.AddShader(&mVertShaderShadow);
	mPassShadowBlue.AddShader(&mFragShaderShadow);

	mPassShadowBlurToH.SetCamera(&mCameraDeferred);
	mPassShadowBlurToH.AddMesh(&mMeshFullScreenQuad);
	mPassShadowBlurToH.AddTexture(&mRenderTextureDiffuse);//from input
	mPassShadowBlurToH.AddRenderTexture(&mRenderTextureBlurH);//to H
	mPassShadowBlurToH.AddShader(&mVertShaderDeferred);
	mPassShadowBlurToH.AddShader(&mFragShaderBlurH);

	mPassShadowBlurFromHToV.SetCamera(&mCameraDeferred);
	mPassShadowBlurFromHToV.AddMesh(&mMeshFullScreenQuad);
	mPassShadowBlurFromHToV.AddTexture(&mRenderTextureBlurH);//from H
	mPassShadowBlurFromHToV.AddRenderTexture(&mRenderTextureBlurV);//to V
	mPassShadowBlurFromHToV.AddShader(&mVertShaderDeferred);
	mPassShadowBlurFromHToV.AddShader(&mFragShaderBlurV);

	mPassShadowBlurFromVToH.SetCamera(&mCameraDeferred);
	mPassShadowBlurFromVToH.AddMesh(&mMeshFullScreenQuad);
	mPassShadowBlurFromVToH.AddTexture(&mRenderTextureBlurV);//from V
	mPassShadowBlurFromVToH.AddRenderTexture(&mRenderTextureBlurH);//to H
	mPassShadowBlurFromVToH.AddShader(&mVertShaderDeferred);
	mPassShadowBlurFromVToH.AddShader(&mFragShaderBlurH);

	mPassShadowBlurFromV.SetCamera(&mCameraDeferred);
	mPassShadowBlurFromV.AddMesh(&mMeshFullScreenQuad);
	mPassShadowBlurFromV.AddTexture(&mRenderTextureBlurV);//from V
	mPassShadowBlurFromV.AddRenderTexture(&mRenderTextureDiffuse);//to output
	mPassShadowBlurFromV.AddShader(&mVertShaderDeferred);
	mPassShadowBlurFromV.AddShader(&mFragShaderBlurV);
}

void CreateScenes()
{
	mScene.AddPass(&mPassDeferred);
	mScene.AddPass(&mPassSkin);
	mScene.AddPass(&mPassStandard);
	mScene.AddPass(&mPassShadowRed);
	mScene.AddPass(&mPassShadowGreen);
	mScene.AddPass(&mPassShadowBlue);
	mScene.AddLight(&mLightRed);
	mScene.AddLight(&mLightGreen);
	mScene.AddLight(&mLightBlue);
}

void CreateLevels()
{
	mLevel.AddLight(&mLightRed);
	mLevel.AddLight(&mLightGreen);
	mLevel.AddLight(&mLightBlue);
	mLevel.AddCamera(&mCameraDeferred);
	mLevel.AddCamera(&mCameraOffscreen);
	mLevel.AddCamera(&mCameraRedLight);
	mLevel.AddCamera(&mCameraGreenLight);
	mLevel.AddCamera(&mCameraBlueLight);
	mLevel.AddMesh(&mMeshHead);
	mLevel.AddMesh(&mMeshFullScreenQuad);
	mLevel.AddMesh(&mMeshSquareX);
	mLevel.AddMesh(&mMeshSquareY);
	mLevel.AddMesh(&mMeshSquareZ);
	mLevel.AddPass(&mPassDeferred);
	mLevel.AddPass(&mPassSkin);
	mLevel.AddPass(&mPassStandard);
	mLevel.AddPass(&mPassShadowRed);
	mLevel.AddPass(&mPassShadowGreen);
	mLevel.AddPass(&mPassShadowBlue);
	mLevel.AddScene(&mScene);
	mLevel.AddShader(&mVertShaderSkin);
	mLevel.AddShader(&mFragShaderSkin);
	mLevel.AddShader(&mVertShaderStandard);
	mLevel.AddShader(&mFragShaderStandard);
	mLevel.AddShader(&mVertShaderDeferred);
	mLevel.AddShader(&mFragShaderDeferred);
	mLevel.AddShader(&mVertShaderShadow);
	mLevel.AddShader(&mFragShaderShadow);
	mLevel.AddTexture(&mTextureColorSkin);
	mLevel.AddTexture(&mTextureNormalSkin);
	mLevel.AddTexture(&mTextureColorBrick);
	mLevel.AddTexture(&mTextureNormalBrick);
	mLevel.AddTexture(&mRenderTextureDiffuse);
	mLevel.AddTexture(&mRenderTextureSpecular);
	mLevel.AddTexture(&mRenderTextureRedLight);
	mLevel.AddTexture(&mRenderTextureGreenLight);
	mLevel.AddTexture(&mRenderTextureBlueLight);
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
		mPassSkin.UpdatePassUniformBuffer(&mCameraOffscreen);
		mPassStandard.UpdatePassUniformBuffer(&mCameraOffscreen);
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
		mPassSkin.UpdatePassUniformBuffer(&mCameraOffscreen);
		mPassStandard.UpdatePassUniformBuffer(&mCameraOffscreen);
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
		mRenderer.skinPipeline,
		mRenderer.skinPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPassSkin);

	mRenderer.CreatePipeline(
		mRenderer.standardPipeline,
		mRenderer.standardPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPassStandard);

	mRenderer.CreatePipeline(
		mRenderer.deferredPipeline,
		mRenderer.deferredPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPassDeferred);

	//mPassShadowRed, mPassShadowGreen, mPassShadowBlue share one pipeline
	mRenderer.CreatePipeline(
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.GetLargestFrameDescriptorSetLayout(),
		mPassShadowRed);
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
	static int offscreenMode = 0;
	static int deferredMode = 0;
	static float m = 0.1f;
	static float rho_s = 1.0f;
	bool updateScene = false;

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("SSSSS");

	ImGui::Text("Hold c and use mouse to manipulate camera.");

	if (ImGui::SliderInt("offscreenMode", &offscreenMode, 0, 1))
	{
		mScene.sUBO.offscreenMode = offscreenMode;
		updateScene = true;
	}

	if (ImGui::SliderInt("deferredMode", &deferredMode, 0, 1))
	{
		mScene.sUBO.deferredMode = deferredMode;
		updateScene = true;
	}

	if (ImGui::SliderFloat("m", &m, 0.0f, 1.0f))
	{
		mScene.sUBO.m = m;
		updateScene = true;
	}

	if (ImGui::SliderFloat("rho_s", &rho_s, 0.0f, 1.0f))
	{
		mScene.sUBO.rho_s = rho_s;
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
	
	//1.shadow pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.shadowPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::FRAME),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.shadowPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::SCENE),
		1,
		mScene.GetSceneDescriptorSetPtr(),
		0,
		nullptr);

	//shadow pass red

	mRenderTextureRedLight.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);
	
	mRenderer.RecordCommand(
		mPassShadowRed,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow pass green

	mRenderTextureGreenLight.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);
	
	mRenderer.RecordCommand(
		mPassShadowGreen,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow pass blue

	mRenderTextureBlueLight.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);

	mRenderer.RecordCommand(
		mPassShadowBlue,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow map read

	mRenderTextureRedLight.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);
	mRenderTextureGreenLight.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);
	mRenderTextureBlueLight.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);

	//G buffer write

	mRenderTextureDiffuse.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);
	mRenderTextureSpecular.TransitionLayoutToWrite(mRenderer.defaultCommandBuffers[imageIndex]);

	//2.skin pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.skinPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::FRAME),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.skinPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::SCENE),
		1,
		mScene.GetSceneDescriptorSetPtr(),
		0,
		nullptr);

	mRenderer.RecordCommand(
		mPassSkin,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.skinPipeline,
		mRenderer.skinPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent,
		glm::vec4(0.8, 0.7, 0.8, 1.0),
		glm::vec2(1.0, 0));

	//3.standard pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.standardPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::FRAME),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.standardPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::SCENE),
		1,
		mScene.GetSceneDescriptorSetPtr(),
		0,
		nullptr);

	mRenderer.RecordCommand(
		mPassStandard,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.standardPipeline,
		mRenderer.standardPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent);

	//G buffer read

	mRenderTextureDiffuse.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);
	mRenderTextureSpecular.TransitionLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);

	//4.deferred pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.deferredPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::FRAME),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.deferredPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::SCENE),
		1,
		mScene.GetSceneDescriptorSetPtr(),
		0,
		nullptr);

	mRenderer.RecordCommandNoEnd(
		glm::vec4(0.3, 0.6, 1.0, 1.0),
		glm::vec2(1.0, 0),
		mPassDeferred,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.deferredPipeline,
		mRenderer.deferredPipelineLayout,
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
