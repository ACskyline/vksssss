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
	samplerInfo.magFilter =	VK_FILTER_LINEAR;//VK_FILTER_NEAREST;// 
	samplerInfo.minFilter = VK_FILTER_LINEAR;//VK_FILTER_NEAREST;// 
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

RenderTexture::RenderTexture(const std::string& _name, int _width, int _height, uint32_t _mipLevels, VkFormat _colorFormat, bool _supportDepth, bool _supportMsaa)
	: Texture(_name, _colorFormat), supportDepth(_supportDepth), supportMsaa(_supportMsaa)
{
	width = _width;
	height = _height;
	mipLevels = _mipLevels;
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
		vkDestroyImage(pRenderer->GetDevice(), preResolveImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), preResolveImageMemory, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), preResolveImageView, nullptr);
		vkDestroyImage(pRenderer->GetDevice(), depthImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), depthImageMemory, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), depthImageView, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), colorImageView, nullptr);
		Texture::CleanUp();
		pRenderer = nullptr;//undecessary
	}
}

VkSampleCountFlagBits RenderTexture::GetMsaaSamples() const
{
	return msaaSamples;
}

VkAttachmentDescription RenderTexture::GetColorAttachment(bool clear) const
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = textureFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT,//no msaa;
	//if msaa is supported, this is the resolve destination which does not require clear, 
	//beacause resolve is a full screen operation, but if msaa is not supported, 
	//this is the final result, clear is needed.
	colorAttachment.loadOp = supportMsaa ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : (clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD);
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;// VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return colorAttachment;
}

VkAttachmentDescription RenderTexture::GetDepthAttachment(bool clear) const
{
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = supportMsaa ? msaaSamples : VK_SAMPLE_COUNT_1_BIT,//msaa
	depthAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;// VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;// VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return depthAttachment;
}

VkAttachmentDescription RenderTexture::GetPreResolveAttachment(bool clear) const
{
	VkAttachmentDescription preResolveAttachment = {};
	preResolveAttachment.format = textureFormat;
	preResolveAttachment.samples = msaaSamples;//msaa;
	preResolveAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	preResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	preResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	preResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	preResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;// VK_IMAGE_LAYOUT_UNDEFINED;
	preResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return preResolveAttachment;
}

VkImageView RenderTexture::GetColorImageView() const
{
	return colorImageView;
}

VkImageView RenderTexture::GetDepthImageView() const
{
	return depthImageView;
}

VkImageView RenderTexture::GetPreResolveImageView() const
{
	return preResolveImageView;
}

bool RenderTexture::SupportDepth()
{
	return supportDepth;
}

bool RenderTexture::SupportMsaa()
{
	return supportMsaa;
}

void RenderTexture::CreateRenderTextureImage()
{
	//pre resolve
	if (supportMsaa)
	{
		//update msaa samples
		msaaSamples = pRenderer->FindMaxUsableSampleCount();
		pRenderer->CreateImage(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			1,//pre resolve buffer does not need mip map
			msaaSamples,//msaa
			textureFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			preResolveImage,
			preResolveImageMemory);
		preResolveImageView = pRenderer->CreateImageView(preResolveImage, textureFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, preResolveImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	}

	//depth
	if (supportDepth)
	{
		depthFormat = pRenderer->FindDepthFormat();
		pRenderer->CreateImage(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			1,//depth buffer does not need mip map
			supportMsaa ? msaaSamples : VK_SAMPLE_COUNT_1_BIT,//msaa
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage,
			depthImageMemory);
		depthImageView = pRenderer->CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	}

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
	currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void RenderTexture::TransitionLayoutToWrite(VkCommandBuffer commandBuffer)
{

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.image = textureImage;
	barrier.srcQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.dstQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = mipLevels;

	VkPipelineStageFlags sceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	vkCmdPipelineBarrier(
		commandBuffer, 
		sceStage, dstStage,
		0, 
		0, nullptr,
		0, nullptr, 
		1, &barrier);

	currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void RenderTexture::TransitionLayoutToRead(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.image = textureImage;
	barrier.srcQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.dstQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = mipLevels;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}