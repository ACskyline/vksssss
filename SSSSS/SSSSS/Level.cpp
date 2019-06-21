#include "Level.h"

#include "Camera.h"
#include "Texture.h"
#include "Mesh.h"
#include "Pass.h"
#include "Scene.h"

Level::Level(const std::string& _name) :
	name(_name)
{
}

Level::~Level()
{
}

void Level::AddPass(Pass* pPass)
{
	pPassVec.push_back(pPass);
}

void Level::AddScene(Scene* pScene)
{
	pSceneVec.push_back(pScene);
}

void Level::AddMesh(Mesh* pMesh)
{
	pMeshVec.push_back(pMesh);
}

void Level::AddTexture(Texture* pTexture)
{
	pTextureVec.push_back(pTexture);
}

void Level::AddShader(Shader* pShader)
{
	pShaderVec.push_back(pShader);
}

void Level::AddCamera(Camera* pCamera)
{
	pCameraVec.push_back(pCamera);
}

std::vector<Pass*>& Level::GetPassVec()
{
	return pPassVec;
}

std::vector<Scene*>& Level::GetSceneVec()
{
	return pSceneVec;
}

std::vector<Mesh*>& Level::GetMeshVec()
{
	return pMeshVec;
}

std::vector<Texture*>& Level::GetTextureVec()
{
	return pTextureVec;
}

std::vector<Shader*>& Level::GetShaderVec()
{
	return pShaderVec;
}

std::vector<Camera*>& Level::GetCameraVec()
{
	return pCameraVec;
}

void Level::InitLevel(
	Renderer* pRenderer,
	VkDescriptorPool descriptorPool)
{
	for (auto pCamera : pCameraVec)
		pCamera->InitCamera();

	for (auto pShader : pShaderVec)
		pShader->InitShader(pRenderer);

	for (auto pTexture : pTextureVec)
		pTexture->InitTexture(pRenderer);

	for (auto pMesh : pMeshVec)
		pMesh->InitMesh(pRenderer, descriptorPool);

	for (auto pScene : pSceneVec)
		pScene->InitScene(pRenderer, descriptorPool);

	for (auto pPass : pPassVec)
		pPass->InitPass(pRenderer, descriptorPool);
}

void Level::CleanUp()
{
	for (auto pCamera : pCameraVec)
		pCamera->CleanUp();

	for (auto pShader : pShaderVec)
		pShader->CleanUp();

	for (auto pTexture : pTextureVec)
		pTexture->CleanUp();

	for (auto pMesh : pMeshVec)
		pMesh->CleanUp();

	for (auto pScene : pSceneVec)
		pScene->CleanUp();

	for (auto pPass : pPassVec)
		pPass->CleanUp();
}