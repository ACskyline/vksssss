#include "Texture.h"

#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(const std::string& _fileName, VkFormat _textureFormat) :
	pRenderer(nullptr), fileName(_fileName), textureFormat(_textureFormat)
{
}

Texture::~Texture()
{
	CleanUp();
}

VkSampler Texture::GetSampler() const
{
	return textureSampler;
}

int Texture::GetWidth() const
{
	return width;
}

int Texture::GetHeight() const
{
	return height;
}

VkImageView Texture::GetTextureImageView() const
{
	return textureImageView;
}

void Texture::InitTexture(Renderer* _pRenderer)
{
	if(_pRenderer==nullptr)
	{
		throw std::runtime_error("texture " + fileName + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
}

void Texture::CreateTextureImage()
{
	int texChannels;
	stbi_uc* pixels = stbi_load(fileName.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = width * height * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	pRenderer->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(pRenderer->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(pRenderer->GetDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	//Our texture image now has multiple mip levels, but the staging buffer can only be used to fill mip level 0. 
	//The other levels are still undefined.To fill these levels we need to generate the data from the single level that we have.
	//We will use the vkCmdBlitImage command. VkCmdBlit is considered a transfer operation, 
	//so we must inform Vulkan that we intend to use the texture image as both the source and destination of a transfer.
	//Add VK_IMAGE_USAGE_TRANSFER_SRC_BIT to the texture image's usage flags
	pRenderer->CreateImage(
		width, 
		height, 
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		textureFormat,//VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage, 
		textureImageMemory);

	//helper functions
	pRenderer->TransitionImageLayout(
		pRenderer->defaultCommandPool, 
		textureImage, 
		textureFormat,//VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		mipLevels);

	pRenderer->CopyBufferToImage(pRenderer->defaultCommandPool, stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	vkDestroyBuffer(pRenderer->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pRenderer->GetDevice(), stagingBufferMemory, nullptr);

	pRenderer->GenerateMipmaps(
		pRenderer->defaultCommandPool,
		textureImage, 
		textureFormat,//VK_FORMAT_R8G8B8A8_UNORM,
		width, 
		height, 
		mipLevels);
}

void Texture::CreateTextureImageView()
{
	textureImageView = pRenderer->CreateImageView(
		textureImage, 
		textureFormat,//VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT, 
		mipLevels);
}

void Texture::CreateTextureSampler() 
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;//tri-linear
	samplerInfo.minLod = 0; // Optional
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.mipLodBias = 0; // Optional

	if (vkCreateSampler(pRenderer->GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void Texture::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroySampler(pRenderer->GetDevice(), textureSampler, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), textureImageView, nullptr);
		vkDestroyImage(pRenderer->GetDevice(), textureImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), textureImageMemory, nullptr);
		pRenderer = nullptr;
	}
}

////////////////////////////////
//////// Render Texture ////////
////////////////////////////////

RenderTexture::RenderTexture(const std::string& _name, int _width, int _height, uint32_t _mipLevel, VkFormat _colorFormat, bool _supportDepth)
	: Texture(_name, _colorFormat), supportDepth(_supportDepth)
{
	width = _width;
	height = _height;
	mipLevels = _mipLevel;
}

RenderTexture::~RenderTexture()
{
	CleanUp();
}

void RenderTexture::InitTexture(Renderer* _pRenderer)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("texture " + fileName + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	CreateRenderTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
}

void RenderTexture::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyImageView(pRenderer->GetDevice(), colorImageView, nullptr);
		vkDestroyImage(pRenderer->GetDevice(), depthImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), depthImageMemory, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), depthImageView, nullptr);
		pRenderer = nullptr;
	}
}

VkAttachmentDescription RenderTexture::GetColorAttachment() const
{
	return colorAttachment;
}

VkAttachmentDescription RenderTexture::GetDepthAttachment() const
{
	return depthAttachment;
}

VkImageView RenderTexture::GetColorImageView() const
{
	return colorImageView;
}

VkImageView RenderTexture::GetDepthImageView() const
{
	return depthImageView;
}

bool RenderTexture::SupportDepth()
{
	return supportDepth;
}

void RenderTexture::CreateRenderTextureImage()
{
	//color
	pRenderer->CreateImage(
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		mipLevels,
		VK_SAMPLE_COUNT_1_BIT,//no msaa
		textureFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory);
	colorImageView = pRenderer->CreateImageView(textureImage, textureFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	
	colorAttachment.format = textureFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT,//no msaa;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//depth
	if (supportDepth)
	{
		depthFormat = pRenderer->FindDepthFormat();
		pRenderer->CreateImage(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			1,//depth buffer does not need mip map
			VK_SAMPLE_COUNT_1_BIT,//no msaa
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage,
			depthImageMemory);
		depthImageView = pRenderer->CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT,//no msaa;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
}
