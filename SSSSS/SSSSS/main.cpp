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
const int WIDTH = 1600;
const int HEIGHT = 900;
const int WIDTH_RT = 1600;
const int HEIGHT_RT = 900;
const int WIDTH_SHADOW_MAP = 2048;
const int HEIGHT_SHADOW_MAP = 2048;
const int MAX_FRAMES_COUNT = IMGUI_VK_QUEUED_FRAMES;//set to 3
const int MAX_BLUR_COUNT = 6;
const uint32_t SKIN_STENCIL_VALUE = 1;
const glm::vec4 CLEAR_COLOR(0.45f, 0.55f, 0.60f, 1.00f);

Renderer mRenderer(WIDTH, HEIGHT, MAX_FRAMES_COUNT);
Level mLevel("default level");
Scene mScene("default scene");
Pass mPassDeferred("deferred pass", true);
Pass mPassSkin("skin pass", true, true, true, true, true, true, VK_COMPARE_OP_ALWAYS, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, SKIN_STENCIL_VALUE);
//Pass mPassStandard("standard pass", false);
Pass mPassShadowRed("shadow pass red", true);
Pass mPassShadowGreen("shadow pass green", true);
Pass mPassShadowBlue("shadow pass blue", true);
std::vector<std::vector<Pass>> mPassBlurVec(static_cast<int>(BLUR_TYPE::Count), std::vector<Pass>(MAX_BLUR_COUNT, Pass("blur", true, false, false, false, false, true, VK_COMPARE_OP_EQUAL, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, SKIN_STENCIL_VALUE)));//[BLUR_TYPE][BLUR_COUNT]
Shader mVertShaderSkin(Shader::ShaderType::VertexShader, "skin.vert");
Shader mFragShaderSkin(Shader::ShaderType::FragmentShader, "skinTSM.frag");
//Shader mVertShaderStandard(Shader::ShaderType::VertexShader, "standard.vert");
//Shader mFragShaderStandard(Shader::ShaderType::FragmentShader, "standard.frag");
Shader mVertShaderDeferred(Shader::ShaderType::VertexShader, "deferred.vert");
Shader mFragShaderDeferred(Shader::ShaderType::FragmentShader, "deferred.frag");
//Shader mVertShaderShadow(Shader::ShaderType::VertexShader, "shadow.vert");
Shader mFragShaderTSM(Shader::ShaderType::FragmentShader, "shadowTSM.frag");
Shader mFragShaderBlurH(Shader::ShaderType::FragmentShader, "blurh.frag");
Shader mFragShaderBlurV(Shader::ShaderType::FragmentShader, "blurv.frag");
Camera mCameraDeferred("deferred camera", glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH, HEIGHT, 0.1f, 50.f);
Camera mCameraRedLight("red light camera", glm::vec3(5, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
Camera mCameraGreenLight("green light camera", glm::vec3(0, 5, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
Camera mCameraBlueLight("blue light camera", glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, 0.1f, 50.f);
OrbitCamera mCameraOffscreen("offscreen camera", 3, 45.f, 45.f, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 45.f, WIDTH_RT, HEIGHT_RT, 0.1f, 50.f);
OrbitCamera* pCurrentOrbitCamera = &mCameraOffscreen;
Mesh mMeshHead("head_smooth.obj", Mesh::MeshType::File, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
//Mesh mMeshSquareX("square x", Mesh::MeshType::Square, glm::vec3(-3, 0, 0), glm::vec3(0, 90, 0), glm::vec3(6, 6, 1));
//Mesh mMeshSquareY("square y", Mesh::MeshType::Square, glm::vec3(0, -3, 0), glm::vec3(-90, 0, 0), glm::vec3(6, 6, 1));
//Mesh mMeshSquareZ("square z", Mesh::MeshType::Square, glm::vec3(0, 0, -3), glm::vec3(0, 0, 0), glm::vec3(6, 6, 1));
Mesh mMeshFullScreenQuad("quad", Mesh::MeshType::FullScreenQuad, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
Texture mTextureColorSkin("head_color.jpg", VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp);
Texture mTextureNormalSkin("head_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp);
Texture mTextureColorBrick("brick_color.png", VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp);
Texture mTextureNormalBrick("brick_normal.png", VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp);
Texture mTextureTransmitanceMask("head_thickness.jpg", VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp);
RenderTexture mRenderTextureDiffuse("diffuse rt", WIDTH_RT, HEIGHT_RT, VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Bilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color);
RenderTexture mRenderTextureSpecular("specular rt", WIDTH_RT, HEIGHT_RT, VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Bilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color);
RenderTexture mRenderTextureDepthStencil("depthStencil rt", WIDTH_RT, HEIGHT_RT, VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Bilinear, Texture::Wrap::Clamp, false, true, false, RenderTexture::ReadFrom::None);
RenderTexture mRenderTextureRedLight("red light rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_UNDEFINED, Texture::Filter::NearestPoint, Texture::Wrap::Clamp, false, true, false, RenderTexture::ReadFrom::Depth);
RenderTexture mRenderTextureGreenLight("green light rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_UNDEFINED, Texture::Filter::NearestPoint, Texture::Wrap::Clamp, false, true, false, RenderTexture::ReadFrom::Depth);
RenderTexture mRenderTextureBlueLight("blue light rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_UNDEFINED, Texture::Filter::NearestPoint, Texture::Wrap::Clamp, false, true, false, RenderTexture::ReadFrom::Depth);
RenderTexture mRenderTextureRedLightTSM("red light tsm rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_R16G16B16A16_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color);
RenderTexture mRenderTextureGreenLightTSM("green light tsm rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_R16G16B16A16_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color);
RenderTexture mRenderTextureBlueLightTSM("blue light tsm rt", WIDTH_SHADOW_MAP, HEIGHT_SHADOW_MAP, VK_FORMAT_R16G16B16A16_UNORM, Texture::Filter::Trilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color);
std::vector<std::vector<RenderTexture>> mRenderTextureBlurVec(static_cast<int>(BLUR_TYPE::Count), std::vector<RenderTexture>(MAX_BLUR_COUNT, RenderTexture("blur", WIDTH_RT, HEIGHT_RT, VK_FORMAT_R8G8B8A8_UNORM, Texture::Filter::Bilinear, Texture::Wrap::Clamp, true, false, false, RenderTexture::ReadFrom::Color)));//[BLUR_TYPE][BLUR_COUNT]
Light mLightRed("light red", glm::vec3(0.8f, 0.4f, 0.4f), glm::vec3(5, 0, 0), &mCameraRedLight, &mRenderTextureRedLight, &mRenderTextureRedLightTSM);
Light mLightGreen("light green", glm::vec3(0.4f, 0.8f, 0.4f), glm::vec3(0, 5, 0), &mCameraGreenLight, &mRenderTextureGreenLight, &mRenderTextureGreenLightTSM);
Light mLightBlue("light blue", glm::vec3(0.4f, 0.4f, 0.8f), glm::vec3(0, 0, 5), &mCameraBlueLight, &mRenderTextureBlueLight, &mRenderTextureBlueLightTSM);

static int newFrame = 0;
static int updateSettings = 0;
static int updateCamera = 0;
static int updateHead = 0;

//imgui stuff
VkDescriptorPool ImGuiDescriptorPool;
bool show_demo_window = true;
bool show_another_window = false;

//io stuff
bool MOUSE_LEFT_BUTTON_DOWN = false;
bool MOUSE_RIGHT_BUTTON_DOWN = false;
bool MOUSE_MIDDLE_BUTTON_DOWN = false;

void CreatePasses()
{
	//shadow
	mPassShadowRed.SetCamera(&mCameraRedLight);
	mPassShadowRed.AddMesh(&mMeshHead);
	//mPassShadowRed.AddMesh(&mMeshSquareX);
	//mPassShadowRed.AddMesh(&mMeshSquareY);
	//mPassShadowRed.AddMesh(&mMeshSquareZ);
	mPassShadowRed.AddRenderTexture(&mRenderTextureRedLight);
	mPassShadowRed.AddRenderTexture(&mRenderTextureRedLightTSM);
	mPassShadowRed.AddShader(&mVertShaderSkin);
	mPassShadowRed.AddShader(&mFragShaderTSM);

	mPassShadowGreen.SetCamera(&mCameraGreenLight);
	mPassShadowGreen.AddMesh(&mMeshHead);
	//mPassShadowGreen.AddMesh(&mMeshSquareX);
	//mPassShadowGreen.AddMesh(&mMeshSquareY);
	//mPassShadowGreen.AddMesh(&mMeshSquareZ);
	mPassShadowGreen.AddRenderTexture(&mRenderTextureGreenLight);
	mPassShadowGreen.AddRenderTexture(&mRenderTextureGreenLightTSM);
	mPassShadowGreen.AddShader(&mVertShaderSkin);
	mPassShadowGreen.AddShader(&mFragShaderTSM);

	mPassShadowBlue.SetCamera(&mCameraBlueLight);
	mPassShadowBlue.AddMesh(&mMeshHead);
	//mPassShadowBlue.AddMesh(&mMeshSquareX);
	//mPassShadowBlue.AddMesh(&mMeshSquareY);
	//mPassShadowBlue.AddMesh(&mMeshSquareZ);
	mPassShadowBlue.AddRenderTexture(&mRenderTextureBlueLight);
	mPassShadowBlue.AddRenderTexture(&mRenderTextureBlueLightTSM);
	mPassShadowBlue.AddShader(&mVertShaderSkin);
	mPassShadowBlue.AddShader(&mFragShaderTSM);

	//skin
	mPassSkin.SetCamera(&mCameraOffscreen);
	mPassSkin.AddMesh(&mMeshHead);
	mPassSkin.AddTexture(&mTextureColorSkin);
	mPassSkin.AddTexture(&mTextureNormalSkin);
	mPassSkin.AddTexture(&mTextureTransmitanceMask);
	mPassSkin.AddRenderTexture(&mRenderTextureDiffuse);
	mPassSkin.AddRenderTexture(&mRenderTextureSpecular);
	mPassSkin.AddRenderTexture(&mRenderTextureDepthStencil);//stencil write
	mPassSkin.AddShader(&mVertShaderSkin);
	mPassSkin.AddShader(&mFragShaderSkin);

	//other
	//mPassStandard.SetCamera(&mCameraOffscreen);
	//mPassStandard.AddMesh(&mMeshSquareX);
	//mPassStandard.AddMesh(&mMeshSquareY);
	//mPassStandard.AddMesh(&mMeshSquareZ);
	//mPassStandard.AddTexture(&mTextureColorBrick);
	//mPassStandard.AddTexture(&mTextureNormalBrick);
	//mPassStandard.AddRenderTexture(&mRenderTextureDiffuse);
	//mPassStandard.AddRenderTexture(&mRenderTextureSpecular);
	//mPassStandard.AddRenderTexture(&mRenderTextureDepthStencil);
	//mPassStandard.AddShader(&mVertShaderStandard);
	//mPassStandard.AddShader(&mFragShaderStandard);

	//blur
	//first blur
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].SetCamera(&mCameraDeferred);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddMesh(&mMeshFullScreenQuad);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddTexture(&mRenderTextureDiffuse);//from diffuse
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddTexture(&mRenderTextureDiffuse);//input should also include depth
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddRenderTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0]);//to h
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddRenderTexture(&mRenderTextureDepthStencil);//stencil test
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddShader(&mVertShaderDeferred);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0].AddShader(&mFragShaderBlurH);

	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].SetCamera(&mCameraDeferred);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddMesh(&mMeshFullScreenQuad);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][0]);//from h
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddTexture(&mRenderTextureDiffuse);//input should also include depth
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddRenderTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0]);//to v
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddRenderTexture(&mRenderTextureDepthStencil);//stencil test
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddShader(&mVertShaderDeferred);
	mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][0].AddShader(&mFragShaderBlurV);
	
	//sequential blur
	for (int i = 1; i < MAX_BLUR_COUNT; i++)
	{
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].SetCamera(&mCameraDeferred);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddMesh(&mMeshFullScreenQuad);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i - 1]);//from v
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddTexture(&mRenderTextureDiffuse);//input should also include depth
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddRenderTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i]);//to h
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddRenderTexture(&mRenderTextureDepthStencil);//stencil test
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddShader(&mVertShaderDeferred);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].AddShader(&mFragShaderBlurH);

		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].SetCamera(&mCameraDeferred);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddMesh(&mMeshFullScreenQuad);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i]);//from h
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddTexture(&mRenderTextureDiffuse);//input should also include depth
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddRenderTexture(&mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i]);//to v
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddRenderTexture(&mRenderTextureDepthStencil);//stencil test
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddShader(&mVertShaderDeferred);
		mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].AddShader(&mFragShaderBlurV);
	}

	//deferred
	mPassDeferred.SetCamera(&mCameraDeferred);
	mPassDeferred.AddMesh(&mMeshFullScreenQuad);
	mPassDeferred.AddTexture(&mRenderTextureDiffuse);
	mPassDeferred.AddTexture(&mRenderTextureSpecular);
	for (auto& mRenderTextureBlurVertical : mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)])
	{
		mPassDeferred.AddTexture(&mRenderTextureBlurVertical);
	}
	mPassDeferred.AddShader(&mVertShaderDeferred);
	mPassDeferred.AddShader(&mFragShaderDeferred);
}

void CreateScenes()
{
	mScene.AddPass(&mPassDeferred);
	mScene.AddPass(&mPassSkin);
	//mScene.AddPass(&mPassStandard);
	mScene.AddPass(&mPassShadowRed);
	mScene.AddPass(&mPassShadowGreen);
	mScene.AddPass(&mPassShadowBlue);
	for (auto& blurPassVec : mPassBlurVec)
	{
		for (auto& blurPass : blurPassVec)
		{
			mScene.AddPass(&blurPass);
		}
	}
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
	//mLevel.AddMesh(&mMeshSquareX);
	//mLevel.AddMesh(&mMeshSquareY);
	//mLevel.AddMesh(&mMeshSquareZ);
	mLevel.AddPass(&mPassDeferred);
	mLevel.AddPass(&mPassSkin);
	//mLevel.AddPass(&mPassStandard);
	mLevel.AddPass(&mPassShadowRed);
	mLevel.AddPass(&mPassShadowGreen);
	mLevel.AddPass(&mPassShadowBlue);
	for (auto& blurPassVec : mPassBlurVec)
	{
		for (auto& blurPass : blurPassVec)
		{
			mLevel.AddPass(&blurPass);
		}
	}
	mLevel.AddScene(&mScene);
	mLevel.AddShader(&mVertShaderSkin);
	mLevel.AddShader(&mFragShaderSkin);
	//mLevel.AddShader(&mVertShaderStandard);
	//mLevel.AddShader(&mFragShaderStandard);
	mLevel.AddShader(&mVertShaderDeferred);
	mLevel.AddShader(&mFragShaderDeferred);
	//mLevel.AddShader(&mVertShaderShadow);
	mLevel.AddShader(&mFragShaderTSM);
	mLevel.AddShader(&mFragShaderBlurH);
	mLevel.AddShader(&mFragShaderBlurV);
	mLevel.AddTexture(&mTextureColorSkin);
	mLevel.AddTexture(&mTextureNormalSkin);
	mLevel.AddTexture(&mTextureTransmitanceMask);
	mLevel.AddTexture(&mTextureColorBrick);
	mLevel.AddTexture(&mTextureNormalBrick);
	mLevel.AddTexture(&mRenderTextureDiffuse);
	mLevel.AddTexture(&mRenderTextureSpecular);
	mLevel.AddTexture(&mRenderTextureDepthStencil);
	mLevel.AddTexture(&mRenderTextureRedLight);
	mLevel.AddTexture(&mRenderTextureGreenLight);
	mLevel.AddTexture(&mRenderTextureBlueLight);
	mLevel.AddTexture(&mRenderTextureRedLightTSM);
	mLevel.AddTexture(&mRenderTextureGreenLightTSM);
	mLevel.AddTexture(&mRenderTextureBlueLightTSM);
	for (auto& blurRTVec : mRenderTextureBlurVec)
	{
		for (auto& blurRT : blurRTVec)
		{
			mLevel.AddTexture(&blurRT);
		}
	}
}

//NOT called one time during each update, so move the update code to one place
void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	const int updateHeadCount = mRenderer.frameCount;

	switch (key)
	{
	case GLFW_KEY_X:
		if (action == GLFW_REPEAT)
		{
			float sign = mods == GLFW_MOD_CONTROL ? -1.f : 1.f;
			mMeshHead.rotation += sign * glm::vec3(1, 0, 0);
			updateHead = updateHeadCount;
		}
		break;
	case GLFW_KEY_Y:
		if (action == GLFW_REPEAT)
		{
			float sign = mods == GLFW_MOD_CONTROL ? -1.f : 1.f;
			mMeshHead.rotation += sign * glm::vec3(0, 1, 0);
			updateHead = updateHeadCount;
		}
		break;
	case GLFW_KEY_Z:
		if (action == GLFW_REPEAT)
		{
			float sign = mods == GLFW_MOD_CONTROL ? -1.f : 1.f;
			mMeshHead.rotation += sign * glm::vec3(0, 0, 1);
			updateHead = updateHeadCount;
		}
		break;
	default:
		break;
	}
}

//NOT called one time during each update, so move the update code to one place
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

//NOT called one time during each update, so move the update code to one place
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
	const int updateCameraCount = mRenderer.frameCount;
	int keyC = glfwGetKey(window, GLFW_KEY_C);
	if (MOUSE_LEFT_BUTTON_DOWN && keyC == GLFW_PRESS)
	{
		if (glm::abs(xDelta) > EPSILON)
		{
			pCurrentOrbitCamera->horizontalAngle += xDelta * horizontalFactor;
			updateCamera = updateCameraCount;
		}
		if (glm::abs(yDelta) > EPSILON)
		{
			pCurrentOrbitCamera->verticalAngle += yDelta * verticalFactor;
			if (pCurrentOrbitCamera->verticalAngle > 90 - EPSILON) pCurrentOrbitCamera->verticalAngle = 89 - EPSILON;
			if (pCurrentOrbitCamera->verticalAngle < -90 + EPSILON) pCurrentOrbitCamera->verticalAngle = -89 + EPSILON;
			updateCamera = updateCameraCount;
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
			updateCamera = updateCameraCount;
		}
	}

	xOld = static_cast<float>(x);
	yOld = static_cast<float>(y);
}

//NOT called one time during each update, so move the update code to one place
void MouseScroll(GLFWwindow* window, double x, double y)
{
	const float zFactor = 0.1f;
	const float& direction = static_cast<float>(y);

	const int updateCameraCount = mRenderer.frameCount;
	int keyC = glfwGetKey(window, GLFW_KEY_C);
	if (keyC == GLFW_PRESS)
	{
		if (glm::abs(direction - EPSILON) > 0)
		{
			pCurrentOrbitCamera->distance -= direction * zFactor;
			if (pCurrentOrbitCamera->distance < 0 + EPSILON) pCurrentOrbitCamera->distance = 0.1f + EPSILON;
			updateCamera = updateCameraCount;
		}
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

	glfwSetKeyCallback(mRenderer.window, Keyboard);
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

	//mRenderer.CreatePipeline(
	//	mRenderer.standardPipeline,
	//	mRenderer.standardPipelineLayout,
	//	mRenderer.GetLargestFrameDescriptorSetLayout(),
	//	mPassStandard);

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

	for (int i = 0; i < static_cast<int>(BLUR_TYPE::Count); i++)
	{
		mRenderer.CreatePipeline(
			mRenderer.blurPipeline[i],
			mRenderer.blurPipelineLayout[i],
			mRenderer.GetLargestFrameDescriptorSetLayout(),
			mPassBlurVec[i][0]);
	}

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

//called one time during each update
void DrawImGui(VkCommandBuffer commandBuffer)
{
	static int deferredMode = mScene.sUBO.deferredMode = 8;
	static int shadowMode = mScene.sUBO.shadowMode = 5;
	static float m = mScene.sUBO.m = 0.114f;
	static float rho_s = mScene.sUBO.rho_s = 0.151f;
	static float stretchAlpha = mScene.sUBO.stretchAlpha = 0.457f;
	static float stretchBeta = mScene.sUBO.stretchBeta = 3000.0f;
	static int tsmMode = mScene.sUBO.tsmMode = 2;
	static float scattering = mScene.sUBO.scattering = 0.6f;
	static float absorption = mScene.sUBO.absorption = 0.6f;
	static float translucencyScale = mScene.sUBO.translucencyScale = 4.232f;
	static float translucencyPower = mScene.sUBO.translucencyPower = 8.620f;
	static float tsmBiasMax = mScene.sUBO.tsmBiasMax = 0.00003f;
	static float tsmBiasMin = mScene.sUBO.tsmBiasMin = 0.005f;
	static float distortion = mScene.sUBO.distortion = 0.266f;
	static float distanceScale = mScene.sUBO.distanceScale = 3.5f;
	static float shadowBias = mScene.sUBO.shadowBias = 0.0001f;
	static float shadowScale = mScene.sUBO.shadowScale = 1.0f;

	const int updateSettingsCount = mRenderer.frameCount;
	static int firstTime = true;//first time update all
	if (firstTime)
	{
		updateSettings = updateSettingsCount;
		firstTime = false;
	}

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("SSSSS");

	ImGui::Text("Hold c and use mouse to manipulate camera.");
	ImGui::Text("Hold x, y and z to rotate head.");

	if (ImGui::SliderInt("deferredMode", &deferredMode, 0, 2 + MAX_BLUR_COUNT))
	{
		mScene.sUBO.deferredMode = deferredMode;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderInt("shadowMode", &shadowMode, 0, 5))
	{
		mScene.sUBO.shadowMode = shadowMode;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("m", &m, 0.0f, 1.0f))
	{
		mScene.sUBO.m = m;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("rho_s", &rho_s, 0.0f, 1.0f))
	{
		mScene.sUBO.rho_s = rho_s;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("stretchAlpha", &stretchAlpha, 0.0f, 2.0f))
	{
		mScene.sUBO.stretchAlpha = stretchAlpha;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("stretchBeta", &stretchBeta, 0.0f, 10000.0f))
	{
		mScene.sUBO.stretchBeta = stretchBeta;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderInt("tsmMode", &tsmMode, 0, 3))
	{
		mScene.sUBO.tsmMode = tsmMode;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("scattering", &scattering, 0.0f, 100.0f, "%.6f"))
	{
		mScene.sUBO.scattering = scattering;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("absorption", &absorption, 0.0f, 100.0f, "%.6f"))
	{
		mScene.sUBO.absorption = absorption;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("translucencyScale", &translucencyScale, 0.0f, 10.0f, "%.6f"))
	{
		mScene.sUBO.translucencyScale = translucencyScale;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("translucencyPower", &translucencyPower, 0.0f, 10.0f, "%.6f"))
	{
		mScene.sUBO.translucencyPower = translucencyPower;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("tsmBiasMax", &tsmBiasMax, 0.0f, 0.01f, "%.6f"))
	{
		mScene.sUBO.tsmBiasMax = tsmBiasMax;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("tsmBiasMin", &tsmBiasMin, 0.0f, 0.01f, "%.6f"))
	{
		mScene.sUBO.tsmBiasMin = tsmBiasMin;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("distortion", &distortion, 0.0f, 1.0f, "%.6f"))
	{
		mScene.sUBO.distortion = distortion;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("distanceScale", &distanceScale, 0.0f, 10.0f, "%.6f"))
	{
		mScene.sUBO.distanceScale = distanceScale;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("shadowBias", &shadowBias, 0.0f, 1.0f, "%.6f"))
	{
		mScene.sUBO.shadowBias = shadowBias;
		updateSettings = updateSettingsCount;
	}

	if (ImGui::SliderFloat("shadowScale", &shadowScale, 0.0f, 1.0f, "%.4f"))
	{
		mScene.sUBO.shadowScale = shadowScale;
		updateSettings = updateSettingsCount;
	}

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	// Rendering
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void DrawBegin()
{
	newFrame = mRenderer.WaitForFence();
	mRenderer.BeginCommandBuffer(mRenderer.defaultCommandBuffers[newFrame]);
	//mRenderer.frameVec[imageIndex].UpdateFrameUniformBuffer();
	
	// 1. shadow pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.shadowPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Frame),
		1,
		mRenderer.frameVec[newFrame].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.shadowPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Scene),
		1,
		mScene.GetSceneDescriptorSetPtr(newFrame),
		0,
		nullptr);

	//shadow pass red

	mRenderTextureRedLight.TransitionDepthStencilLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureRedLightTSM.TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	
	mRenderer.RecordCommand(
		newFrame,
		mPassShadowRed,
		mRenderer.defaultCommandBuffers[newFrame],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[newFrame],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow pass green

	mRenderTextureGreenLight.TransitionDepthStencilLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureGreenLightTSM.TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);

	mRenderer.RecordCommand(
		newFrame,
		mPassShadowGreen,
		mRenderer.defaultCommandBuffers[newFrame],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[newFrame],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow pass blue

	mRenderTextureBlueLight.TransitionDepthStencilLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureBlueLightTSM.TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);

	mRenderer.RecordCommand(
		newFrame,
		mPassShadowBlue,
		mRenderer.defaultCommandBuffers[newFrame],
		mRenderer.shadowPipeline,
		mRenderer.shadowPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[newFrame],
		mRenderer.swapChainExtent,
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec2(1.0, 0));

	//shadow map read

	mRenderTextureRedLight.TransitionDepthStencilLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureGreenLight.TransitionDepthStencilLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureBlueLight.TransitionDepthStencilLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureRedLightTSM.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureGreenLightTSM.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureBlueLightTSM.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);

	// 1.5 genrate mip chain for TSM

	//generate mip chain
	
	mRenderTextureRedLightTSM.TransitionColorLayoutToTransferDst(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureRedLightTSM.GenerateMipMapsAndTransitionColorLayout(mRenderer.defaultCommandBuffers[newFrame], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//depth & G buffer write

	mRenderTextureDepthStencil.TransitionDepthStencilLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureDiffuse.TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderTextureSpecular.TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);

	// 2. skin pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.skinPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Frame),
		1,
		mRenderer.frameVec[newFrame].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.skinPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Scene),
		1,
		mScene.GetSceneDescriptorSetPtr(newFrame),
		0,
		nullptr);

	mRenderer.RecordCommand(
		newFrame,
		mPassSkin,
		mRenderer.defaultCommandBuffers[newFrame],
		mRenderer.skinPipeline,
		mRenderer.skinPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[newFrame],
		mRenderer.swapChainExtent,
		CLEAR_COLOR,
		glm::vec2(1.0, 0));

	/*
	// 3. standard pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.standardPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Frame),
		1,
		mRenderer.frameVec[imageIndex].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[imageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.standardPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Scene),
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

	//depth & diffuse read
	*/

	mRenderTextureDiffuse.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	//mRenderTextureDepthStencil.TransitionDepthStencilLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);

	// 4. blur pipeline

	//bind frame descriptor set
	for (int i = 0; i < static_cast<int>(BLUR_TYPE::Count); i++)
	{
		vkCmdBindDescriptorSets(
			mRenderer.defaultCommandBuffers[newFrame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mRenderer.blurPipelineLayout[i],
			static_cast<uint32_t>(UNIFORM_SLOT::Frame),
			1,
			mRenderer.frameVec[newFrame].GetFrameDescriptorSetPtr(),
			0,
			nullptr);

		//bind scene descriptor set
		vkCmdBindDescriptorSets(
			mRenderer.defaultCommandBuffers[newFrame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			mRenderer.blurPipelineLayout[i],
			static_cast<uint32_t>(UNIFORM_SLOT::Scene),
			1,
			mScene.GetSceneDescriptorSetPtr(newFrame),
			0,
			nullptr);
	}

	//blur multiple times
	for (int i = 0; i < MAX_BLUR_COUNT; i++)
	{
		//write to horizontal rt
		mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);

		mRenderer.RecordCommand(
			newFrame,
			mPassBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i],
			mRenderer.defaultCommandBuffers[newFrame],
			mRenderer.blurPipeline[static_cast<int>(BLUR_TYPE::Horizontal)],
			mRenderer.blurPipelineLayout[static_cast<int>(BLUR_TYPE::Horizontal)],
			mRenderer.swapChainRenderPass,
			mRenderer.swapChainFramebuffers[newFrame],
			mRenderer.swapChainExtent);

		//read from horizontal rt
		mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Horizontal)][i].TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
		//write to vertical rt
		mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].TransitionColorLayoutToWrite(mRenderer.defaultCommandBuffers[newFrame]);

		mRenderer.RecordCommand(
			newFrame,
			mPassBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i],
			mRenderer.defaultCommandBuffers[newFrame],
			mRenderer.blurPipeline[static_cast<int>(BLUR_TYPE::Vertical)],
			mRenderer.blurPipelineLayout[static_cast<int>(BLUR_TYPE::Vertical)],
			mRenderer.swapChainRenderPass,
			mRenderer.swapChainFramebuffers[newFrame],
			mRenderer.swapChainExtent);

		//read from vertical rt
		mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)][i].TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	}

	//specular read
	mRenderTextureSpecular.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[newFrame]);
	
	//uncessary, diffuse is ready to read
	////diffuse read
	//mRenderTextureDiffuse.TransitionColorLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);

	//uncessary, all blur rt are ready to read
	////blur read
	//for (auto& mRenderTextureBlurVertical : mRenderTextureBlurVec[static_cast<int>(BLUR_TYPE::Vertical)])
	//{
	//	mRenderTextureBlurVertical.TransitionDepthLayoutToRead(mRenderer.defaultCommandBuffers[imageIndex]);
	//}

	// 5. deferred pipeline

	//bind frame descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.deferredPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Frame),
		1,
		mRenderer.frameVec[newFrame].GetFrameDescriptorSetPtr(),
		0,
		nullptr);

	//bind scene descriptor set
	vkCmdBindDescriptorSets(
		mRenderer.defaultCommandBuffers[newFrame],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mRenderer.deferredPipelineLayout,
		static_cast<uint32_t>(UNIFORM_SLOT::Scene),
		1,
		mScene.GetSceneDescriptorSetPtr(newFrame),
		0,
		nullptr);

	mRenderer.RecordCommandNoEnd(
		newFrame,
		mPassDeferred,
		mRenderer.defaultCommandBuffers[newFrame],
		mRenderer.deferredPipeline,
		mRenderer.deferredPipelineLayout,
		mRenderer.swapChainRenderPass,
		mRenderer.swapChainFramebuffers[newFrame],
		mRenderer.swapChainExtent,
		CLEAR_COLOR,
		glm::vec2(1.0, 0));

	//render pass remains the same which is swap chain render pass,
	//so this has to be the last render command (which renders to a swap chain framebuffer)
	DrawImGui(mRenderer.defaultCommandBuffers[newFrame]);
}

void DrawEnd()
{
	mRenderer.RecordCommandEnd(mRenderer.defaultCommandBuffers[newFrame]);
	mRenderer.EndCommandBuffer(mRenderer.defaultCommandBuffers[newFrame], newFrame);
}

void UpdateGameLogic()
{
	const int updateHeadCount = mRenderer.frameCount;
	updateHead = updateHeadCount;
	mMeshHead.rotation.y += 0.05;
}

void UpdateUniformBuffers()
{
	if (updateSettings > 0)
	{
		mScene.UpdateSceneUniformBuffer(newFrame);
		updateSettings--;
	}

	if (updateCamera > 0)
	{
		pCurrentOrbitCamera->UpdatePosition();
		mPassSkin.UpdatePassUniformBuffer(newFrame, &mCameraOffscreen);
		updateCamera--;
	}

	if (updateHead > 0)
	{
		mMeshHead.UpdateObjectUniformBuffer(newFrame);
		updateHead--;
	}
}

void MainLoop()
{
	//main loop
	while (!glfwWindowShouldClose(mRenderer.window))
	{
		glfwPollEvents();
		UpdateGameLogic();
		DrawBegin();
		UpdateUniformBuffers();//update CPU data before submit the command buffer
		DrawEnd();
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
