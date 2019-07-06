#pragma once

#include "GlobalInclude.h"

class Renderer;
class Scene;
class Pass;
class Mesh;
class Texture;
class Shader;
class Camera;
class Light;

class Level
{
public:
	Level(const std::string& _name);
	~Level();

	void AddPass(Pass* pPass);
	void AddScene(Scene* pScene);
	void AddMesh(Mesh* pMesh);
	void AddTexture(Texture* pTexture);
	void AddShader(Shader* pShader);
	void AddCamera(Camera* pCamera);
	void AddLight(Light* pLight);

	std::vector<Pass*>& GetPassVec();
	std::vector<Scene*>& GetSceneVec();
	std::vector<Mesh*>& GetMeshVec();
	std::vector<Texture*>& GetTextureVec();

	void InitLevel(
		Renderer* pRenderer, 
		VkDescriptorPool descriptorPool);

	void CleanUp();

private:
	std::string name;

	std::vector<Pass*> pPassVec;
	std::vector<Scene*> pSceneVec;
	std::vector<Mesh*> pMeshVec;
	std::vector<Texture*> pTextureVec;
	std::vector<Shader*> pShaderVec;
	std::vector<Camera*> pCameraVec;
	std::vector<Light*> pLightVec;
};
