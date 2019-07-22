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

	void CreateTextureImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateTextureSampler();

private:
	// ~ texture only functions ~

	void CreateTextureImage();
};

////////////////////////////////
//////// Render Texture ////////
////////////////////////////////

class RenderTexture : public Texture
{
public:
	//RenderTexture();
	RenderTexture(const std::string& _name, int _width, int _height, uint32_t _mipLevels, VkFormat _colorFormat, bool _supportColor, bool _supportDepthStencil, bool _supportMsaa);
	virtual ~RenderTexture();

	void virtual InitTexture(Renderer* _pRenderer);

	void virtual CleanUp();

	VkSampleCountFlagBits GetMsaaSamples() const;
	VkAttachmentDescription GetColorAttachment(bool clear) const;
	VkAttachmentDescription GetDepthAttachment(bool clear) const;
	VkAttachmentDescription GetPreResolveAttachment(bool clear) const;
	VkImageView GetColorImageView() const;
	VkImageView GetDepthImageView() const;
	VkImageView GetPreResolveImageView() const;
	bool SupportColor();
	bool SupportDepthStencil();
	bool SupportMsaa();

	void TransitionColorLayoutToWrite(VkCommandBuffer commandBuffer);
	void TransitionColorLayoutToRead(VkCommandBuffer commandBuffer);

	void TransitionDepthStencilLayoutToWrite(VkCommandBuffer commandBuffer);
	void TransitionDepthStencilLayoutToRead(VkCommandBuffer commandBuffer);

private:

	// ~ color buffer ~

	bool supportColor;
	//VkImage colorImage; //use textureImage instead
	//VkDeviceMemory colorImageMemory; //use textureImageMemory instead
	VkImageView colorImageView;
	VkImageLayout currentColorLayout;

	// ~ depth buffer ~

	bool supportDepthStencil;
	VkImage depthStencilImage;
	VkDeviceMemory depthStencilImageMemory;
	VkImageView depthStencilImageView; 
	VkFormat depthStencilFormat;
	VkImageLayout currentDepthStencilLayout;

	// ~ resolve buffer ~

	bool supportMsaa;
	VkSampleCountFlagBits msaaSamples;
	VkImage preResolveImage;
	VkDeviceMemory preResolveImageMemory;
	VkImageView preResolveImageView;

	// ~ vulkan functions ~

	void CreateRenderTextureImage();
};
