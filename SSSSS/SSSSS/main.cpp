#include "Renderer.h"
#include "Scene.h"
#include <iterator>

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;//3 in total

Renderer mRenderer(WIDTH, HEIGHT, MAX_FRAMES_IN_FLIGHT);
Scene mScene("default scene");
Pass mPass("default pass");
Shader mVertShader(Shader::VertexShader, "standard.vert");
Shader mFragShader(Shader::FragmentShader, "standard.frag");
Camera mCamera("default camera", glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), 45.f, WIDTH, HEIGHT, 0.1f, 10.f);
Mesh mMesh("square", Mesh::MeshType::Square, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Texture mTexture("rex.png", VK_FORMAT_R8G8B8A8_UNORM);

void CreatePasses()
{
	mPass.SetCamera(&mCamera);
	mPass.AddMesh(&mMesh);
	mPass.AddTexture(&mTexture);
}

void CreateScenes()
{
	mScene.AddCamera(&mCamera);
	mScene.AddMesh(&mMesh);
	mScene.AddPass(&mPass);
	mScene.AddShader(&mVertShader);
	mScene.AddShader(&mFragShader);
	mScene.AddTexture(&mTexture);
}

void CreateRenderer()
{
	mRenderer.AddScene(&mScene);
	mRenderer.InitWindow();
	mRenderer.InitVulkan();
	mRenderer.InitDescriptorPool(mRenderer.GetGraphicsDescriptorPoolRef());

	// 1.create layouts

	mRenderer.GetGraphicsDescriptorSetLayoutsRef().resize(static_cast<size_t>(UNIFORM_SLOT::COUNT));
	
	for (int i = 0; i < static_cast<int>(UNIFORM_SLOT::COUNT); i++)
	{
		mRenderer.CreateDescriptorSetLayout(
			mRenderer.GetGraphicsDescriptorSetLayoutRef(i), 
			UniformSlotData[i].uboBindingCount, 
			UniformSlotData[i].uboBindingOffset,
			UniformSlotData[i].textureBindingCount,
			UniformSlotData[i].textureBindingOffset);
	}

	// 2.init assets - depends on layouts

	mRenderer.InitAssets(mRenderer.GetGraphicsDescriptorPool(), mRenderer.GetGraphicsDescriptorSetLayouts());
	
	// 3.create pipeline - must init assests before create pipeline, because shader must be compiled before passed to pipeline
	
	mRenderer.CreatePipeline(
		mRenderer.GetGraphicsPipelineRef(),
		mRenderer.GetGraphicsPipelineLayoutRef(),
		mRenderer.GetGraphicsRenderPass(),
		mRenderer.GetGraphicsDescriptorSetLayouts(),
		&mVertShader,
		nullptr,
		nullptr,
		nullptr,
		&mFragShader
		);

}

int main() {

	try {
		CreatePasses();
		CreateScenes();
		CreateRenderer();
		mRenderer.MainLoop();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
