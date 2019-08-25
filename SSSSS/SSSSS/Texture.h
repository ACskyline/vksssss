#pragma once

#include "GlobalInclude.h"

class Renderer;

class Texture
{
public:

	enum class Filter { NearestPoint, Bilinear, Trilinear };
	enum class Wrap { Clamp, Repeat, Mirror };//clamp to edge

	Texture(const std::string& _fileName, VkFormat _textureFormat, Filter _filter, Wrap _wrap);
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

	Filter filter;
	Wrap wrap;
	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkFormat textureFormat;

	// ~ vulkan functions ~

	void CreateTextureImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateTextureSampler();
	VkFilter GetVkFilter();

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

	enum class ReadFrom { Color, Depth, Stencil, None };

	//RenderTexture();
	RenderTexture(const std::string& _name, int _width, int _height, VkFormat _colorFormat, Filter _filter, Wrap _wrap, bool _supportColor, bool _supportDepthStencil, bool _supportMsaa, ReadFrom _readFrom);
	virtual ~RenderTexture();

	void virtual InitTexture(Renderer* _pRenderer);

	void virtual CleanUp();

	VkSampleCountFlagBits GetMsaaSamples() const;
	VkAttachmentDescription GetColorAttachment(bool clear) const;
	VkAttachmentDescription GetDepthStencilAttachment(bool clearDepth, bool clearStencil) const;
	VkAttachmentDescription GetPreResolveAttachment(bool clear) const;
	VkImageView GetColorImageView() const;
	VkImageView GetDepthStencilImageView() const;
	VkImageView GetPreResolveImageView() const;
	bool SupportColor();
	bool SupportDepthStencil();
	bool SupportMsaa();

	void TransitionColorLayoutToWrite(VkCommandBuffer commandBuffer);
	void TransitionColorLayoutToRead(VkCommandBuffer commandBuffer);

	void TransitionDepthStencilLayoutToWrite(VkCommandBuffer commandBuffer);
	void TransitionDepthStencilLayoutToRead(VkCommandBuffer commandBuffer);

private:

	// ~ general info ~

	ReadFrom readFrom;

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
