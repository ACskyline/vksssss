#pragma once

#include "GlobalInclude.h"

class Renderer;

class Texture
{
public:
	Texture(const std::string& _fileName, VkFormat _textureFormat);
	virtual ~Texture();

	VkImageView GetTextureImageView() const;
	VkSampler GetSampler() const;
	int GetWidth() const;
	int GetHeight() const;

	void virtual InitTexture(Renderer* _pRenderer);

	void virtual CleanUp();

protected:
	Renderer* pRenderer;
	std::string fileName;
	int width;
	int height;

	// ~ texture ~

	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkFormat textureFormat;

	// ~ vulkan functions ~

	void CreateTextureImageView();
	void CreateTextureSampler();

private:
	// ~ texture only functions ~

	void CreateTextureImage();
};

class RenderTexture : public Texture
{
public:
	RenderTexture(const std::string& _name, int _width, int _height, uint32_t _mipLevels, VkFormat _colorFormat, bool _supportDepth, bool _supportMsaa);
	virtual ~RenderTexture();

	void virtual InitTexture(Renderer* _pRenderer);

	void virtual CleanUp();

	VkSampleCountFlagBits GetMsaaSamples() const;
	VkAttachmentDescription GetColorAttachment() const;
	VkAttachmentDescription GetDepthAttachment() const;
	VkAttachmentDescription GetPreResolveAttachment() const;
	VkImageView GetColorImageView() const;
	VkImageView GetDepthImageView() const;
	VkImageView GetPreResolveImageView() const;
	bool SupportDepth();
	bool SupportMsaa();

	void TransitionLayoutToWrite(VkCommandBuffer commandBuffer);
	void TransitionLayoutToRead(VkCommandBuffer commandBuffer);

private:

	// ~ color buffer ~

	//VkImage colorImage; //use textureImage instead
	//VkDeviceMemory colorImageMemory; //use textureImageMemory instead
	VkImageView colorImageView;
	VkAttachmentDescription colorAttachment;
	VkImageLayout currentLayout;

	// ~ depth buffer ~

	bool supportDepth;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView; 
	VkFormat depthFormat;
	VkAttachmentDescription depthAttachment;

	// ~ resolve buffer ~

	bool supportMsaa;
	VkSampleCountFlagBits msaaSamples;
	VkImage preResolveImage;
	VkDeviceMemory preResolveImageMemory;
	VkImageView preResolveImageView;
	VkAttachmentDescription preResolveAttachment;

	// ~ vulkan functions ~

	void CreateRenderTextureImage();
};
