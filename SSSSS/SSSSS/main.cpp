#include "Renderer.h"
#include "Level.h"
#include "Scene.h"
#include "Pass.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Frame.h"

#include <iterator>

const int WIDTH = 800;
const int HEIGHT = 800;
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
Camera mCamera("default camera", glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(1, 1, 0), 45.f, WIDTH, HEIGHT, 0.1f, 10.f);
Camera mCameraOffscreen("offscreen camera", glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH, HEIGHT, 0.1f, 10.f);
Mesh mMesh("square", Mesh::MeshType::Square, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Texture mTexture("rex.png", VK_FORMAT_R8G8B8A8_UNORM);
RenderTexture mRenderTexture("offscreen rt", WIDTH, HEIGHT, 1, VK_FORMAT_R8G8B8A8_UNORM, true, false);

void CreatePasses()
{
	mPassOffscreen.SetCamera(&mCameraOffscreen);
	mPassOffscreen.AddMesh(&mMesh);
	mPassOffscreen.AddTexture(&mTexture);
	mPassOffscreen.AddRenderTexture(&mRenderTexture);
	mPassOffscreen.AddShader(&mVertShader);
	mPassOffscreen.AddShader(&mFragShader);

	mPass.SetCamera(&mCamera);
	mPass.AddMesh(&mMesh);
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
	mLevel.AddMesh(&mMesh);
	mLevel.AddPass(&mPass);
	mLevel.AddPass(&mPassOffscreen);
	mLevel.AddScene(&mScene);
	mLevel.AddShader(&mVertShader);
	mLevel.AddShader(&mFragShader);
	mLevel.AddShader(&mDeferredVertShader);
	mLevel.AddShader(&mDeferredFragShader);
	mLevel.AddTexture(&mTexture);
	mLevel.AddTexture(&mRenderTexture);
}

void CreateWindow() 
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//disable resizing
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mRenderer.window = glfwCreateWindow(WIDTH, HEIGHT, "Yes, Vulkan!", nullptr, nullptr);
	//handle resize explicitly        
	//glfwSetWindowUserPointer(window, &mRenderer);
	//glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
}

void CreateRenderer()
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

void Draw()
{
	uint32_t imageIndex = mRenderer.BeginCommandBuffer(mRenderer.defaultCommandBuffers);
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

	mRenderer.RecordCommand(
		glm::vec4(0.3, 0.6, 1.0, 1.0),
		glm::vec2(1.0, 0),
		mPass,
		mRenderer.defaultCommandBuffers[imageIndex],
		mRenderer.graphicsPipeline,
		mRenderer.graphicsPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[imageIndex],
		mRenderer.swapChainExtent);

	mRenderer.EndCommandBuffer(mRenderer.defaultCommandBuffers, imageIndex);
}

void MainLoop()
{
	//main loop
	while (!glfwWindowShouldClose(mRenderer.window))
	{
		glfwPollEvents();
		Draw();
	}
	mRenderer.IdleWait();
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
		CreateRenderer();
		MainLoop();
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
