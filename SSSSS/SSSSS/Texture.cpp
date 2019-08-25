#include "Texture.h"

#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(const std::string& _fileName, VkFormat _textureFormat, Filter _filter, Wrap _wrap) :
	pRenderer(nullptr), 
	fileName(_fileName), 
	textureFormat(_textureFormat), 
	filter(_filter), 
	wrap(_wrap),
	mipLevels(1),
	textureImage(VK_NULL_HANDLE), 
	textureImageMemory(VK_NULL_HANDLE), 
	textureImageView(VK_NULL_HANDLE),
	textureSampler(VK_NULL_HANDLE)
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
	CreateTextureImageView(textureImage, textureFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	CreateTextureSampler();
}

void Texture::CreateTextureImage()
{
	int texChannels;
	stbi_uc* pixels = stbi_load(fileName.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = width * height * 4;

	if (!pixels) 
	{
		throw std::runtime_error("failed to load texture image!");
	}

	if (filter == Filter::Trilinear)
	{
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	}

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
		textureFormat,
		VK_IMAGE_TILING_OPTIMAL,
		filter == Filter::Trilinear ?
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT :
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage, 
		textureImageMemory);

	//helper functions
	pRenderer->TransitionImageLayout(
		pRenderer->defaultCommandPool, 
		textureImage, 
		textureFormat,
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		mipLevels);

	pRenderer->CopyBufferToImage(pRenderer->defaultCommandPool, stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	vkDestroyBuffer(pRenderer->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pRenderer->GetDevice(), stagingBufferMemory, nullptr);

	if (filter == Filter::Trilinear)
	{
		pRenderer->GenerateMipmaps(
			pRenderer->defaultCommandPool,
			textureImage,
			textureFormat,
			width,
			height,
			mipLevels);
	}
}

void Texture::CreateTextureImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	textureImageView = pRenderer->CreateImageView(
		image,
		format,
		aspectFlags, 
		mipLevels);
}

void Texture::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter =	filter == Filter::NearestPoint ? VK_FILTER_NEAREST : VK_FILTER_LINEAR; 
	samplerInfo.minFilter = filter == Filter::NearestPoint ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.addressModeU = wrap == Wrap::Clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : 
									wrap == Wrap::Mirror ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT :
										wrap == Wrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_REPEAT;//default to repeat
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_TRUE;//for pcf, always on, not sure if it affects performance when unused
	samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;//for pcf, always on, not sure if it affects performance when unused
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;//for trilinear, always on, but it is also controlled by maxLod
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.mipLodBias = 0;

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

RenderTexture::RenderTexture(const std::string& _name, int _width, int _height, VkFormat _colorFormat, Filter _filter, Wrap _wrap, bool _supportColor, bool _supportDepthStencil, bool _supportMsaa, ReadFrom _readFrom)
	: Texture(_name, _colorFormat, _filter, _wrap), 
	supportColor(_supportColor), 
	supportDepthStencil(_supportDepthStencil), 
	supportMsaa(_supportMsaa), 
	readFrom(_readFrom),
	depthStencilImage(VK_NULL_HANDLE),
	depthStencilImageMemory(VK_NULL_HANDLE),
	depthStencilImageView(VK_NULL_HANDLE),
	preResolveImage(VK_NULL_HANDLE),
	preResolveImageMemory(VK_NULL_HANDLE),
	preResolveImageView(VK_NULL_HANDLE)
{
	width = _width;
	height = _height;
	if (filter == Filter::Trilinear)
	{
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
	}
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

	//this is for write
	CreateRenderTextureImage();

	//this is for read
	if (supportColor && readFrom == ReadFrom::Color)
	{
		CreateTextureImageView(textureImage, textureFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	else if (supportDepthStencil && readFrom == ReadFrom::Depth)
	{
		//When using an image view of a depth/stencil image to populate a descriptor set 
		//(e.g. for sampling in the shader, or for use as an input attachment), 
		//the aspectMask must only include one bit and selects whether the image view is used for depth reads 
		//(i.e. using a floating-point sampler or input attachment in the shader) or stencil reads 
		//(i.e. using an unsigned integer sampler or input attachment in the shader). 
		//When an image view of a depth/stencil image is used as a depth/stencil framebuffer attachment, 
		//the aspectMask is ignored and both depth and stencil image subresources are used.
		CreateTextureImageView(depthStencilImage, depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);//no VK_IMAGE_ASPECT_STENCIL_BIT when reading, choose either depth or stencil, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageSubresourceRange.html
	}
	else if (supportDepthStencil && readFrom == ReadFrom::Stencil)
	{
		//When using an image view of a depth/stencil image to populate a descriptor set 
		//(e.g. for sampling in the shader, or for use as an input attachment), 
		//the aspectMask must only include one bit and selects whether the image view is used for depth reads 
		//(i.e. using a floating-point sampler or input attachment in the shader) or stencil reads 
		//(i.e. using an unsigned integer sampler or input attachment in the shader). 
		//When an image view of a depth/stencil image is used as a depth/stencil framebuffer attachment, 
		//the aspectMask is ignored and both depth and stencil image subresources are used.
		CreateTextureImageView(depthStencilImage, depthStencilFormat, VK_IMAGE_ASPECT_STENCIL_BIT);//no VK_IMAGE_ASPECT_DEPTH_BIT when reading, choose either depth or stencil, https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageSubresourceRange.html
	}
	//else
		//nothing

	CreateTextureSampler();
}

void RenderTexture::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyImage(pRenderer->GetDevice(), preResolveImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), preResolveImageMemory, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), preResolveImageView, nullptr);
		vkDestroyImage(pRenderer->GetDevice(), depthStencilImage, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), depthStencilImageMemory, nullptr);
		vkDestroyImageView(pRenderer->GetDevice(), depthStencilImageView, nullptr);
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
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return colorAttachment;
}

VkAttachmentDescription RenderTexture::GetDepthStencilAttachment(bool clearDepth, bool clearStencil) const
{
	VkAttachmentDescription depthStencilAttachment = {};
	depthStencilAttachment.format = depthStencilFormat;
	depthStencilAttachment.samples = supportMsaa ? msaaSamples : VK_SAMPLE_COUNT_1_BIT,//msaa
	depthStencilAttachment.loadOp = clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	depthStencilAttachment.storeOp = supportDepthStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.stencilLoadOp = clearStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;;
	depthStencilAttachment.stencilStoreOp = supportDepthStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return depthStencilAttachment;
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

VkImageView RenderTexture::GetDepthStencilImageView() const
{
	return depthStencilImageView;
}

VkImageView RenderTexture::GetPreResolveImageView() const
{
	return preResolveImageView;
}

bool RenderTexture::SupportColor()
{
	return supportColor;
}

bool RenderTexture::SupportDepthStencil()
{
	return supportDepthStencil;
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

	//depth stencil
	if (supportDepthStencil)
	{
		depthStencilFormat = pRenderer->FindDepthStencilFormat();
		pRenderer->CreateImage(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			mipLevels,
			supportMsaa ? msaaSamples : VK_SAMPLE_COUNT_1_BIT,//msaa
			depthStencilFormat,
			VK_IMAGE_TILING_OPTIMAL,
			readFrom == ReadFrom::Depth || readFrom == ReadFrom::Stencil ?
				(filter == Filter::Trilinear ?
					(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
					:
					(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT))
				:
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthStencilImage,
			depthStencilImageMemory);
		depthStencilImageView = pRenderer->CreateImageView(depthStencilImage, depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 1);
		pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, depthStencilImage, depthStencilFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
		currentDepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	//color
	if (supportColor)
	{
		pRenderer->CreateImage(
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height),
			mipLevels,
			VK_SAMPLE_COUNT_1_BIT,//no msaa
			textureFormat,
			VK_IMAGE_TILING_OPTIMAL,
			readFrom == ReadFrom::Color ? 
				(filter == Filter::Trilinear ? 
					(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
					:
					(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT))
				:
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage,
			textureImageMemory);
		colorImageView = pRenderer->CreateImageView(textureImage, textureFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		pRenderer->TransitionImageLayout(pRenderer->defaultCommandPool, textureImage, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		currentColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
}

void RenderTexture::TransitionColorLayoutToWrite(VkCommandBuffer commandBuffer)
{

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentColorLayout;
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

	currentColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void RenderTexture::TransitionColorLayoutToRead(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentColorLayout;
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

	currentColorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RenderTexture::TransitionDepthStencilLayoutToWrite(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentDepthStencilLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.image = depthStencilImage;
	barrier.srcQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.dstQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = mipLevels;

	VkPipelineStageFlags sceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		sceStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	currentDepthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void RenderTexture::TransitionDepthStencilLayoutToRead(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = currentDepthStencilLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.image = depthStencilImage;
	barrier.srcQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.dstQueueFamilyIndex = pRenderer->GetGraphicsQueueFamilyIndex();
	barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = mipLevels;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	currentDepthStencilLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
