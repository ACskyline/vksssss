#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

#include "GlobalInclude.h"

class Level;
class Pass;
class Frame;
class Shader;

class Renderer
{
public:
	Renderer(int _width, int _hight, int _framesInFlight);
	~Renderer();

	void AddLevel(Level* pLevel);
	void InitVulkan();
	void InitAssets();

	// ~ get general vulkan resources ~

	VkDevice GetDevice() const;
	VkPhysicalDevice GetPhysicalDevice() const;
	VkInstance GetInstance() const;
	VkQueue GetGraphicsQueue() const;
	uint32_t GetGraphicsQueueFamilyIndex() const;
	GLFWwindow* GetWindow() const;
	VkSampleCountFlagBits GetSwapChainMsaaSamples() const;

	// ~ utility ~

	VkFormat FindDepthFormat();
	VkSampleCountFlagBits FindMaxUsableSampleCount();

	// ~ clean up ~

	void IdleWait();
	void CleanUp();

	// ~ general gpu resource operations ~

	void CreateBuffer(
		VkDeviceSize size, 
		VkBufferUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VkBuffer& buffer, 
		VkDeviceMemory& bufferMemory);

	void CopyBuffer(
		VkCommandPool commandPool, 
		VkBuffer srcBuffer, 
		VkBuffer dstBuffer, 
		VkDeviceSize size);

	void CreateImage(
		uint32_t width, 
		uint32_t height, 
		uint32_t mipLevels,
		VkSampleCountFlagBits numSamples,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image, 
		VkDeviceMemory& imageMemory);

	void TransitionImageLayout(
		VkCommandPool commandPool, 
		VkImage image, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t mipLevels);

	void CopyBufferToImage(
		VkCommandPool commandPool, 
		VkBuffer buffer, 
		VkImage image, 
		uint32_t width, 
		uint32_t height);

	void GenerateMipmaps(
		VkCommandPool commandPool, 
		VkImage image, 
		VkFormat imageFormat, 
		int32_t texWidth, 
		int32_t texHeight, 
		uint32_t mipLevels);

	VkImageView CreateImageView(
		VkImage image, 
		VkFormat format, 
		VkImageAspectFlags aspectFlags, 
		uint32_t mipLevels);

	// ~ bind descriptors ~

	void BindUniformBufferToDescriptorSets(VkBuffer buffer, VkDeviceSize size, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t binding);
	//void BindUniformBufferToDescriptorSetsCmd(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkBuffer buffer, VkDeviceSize size, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t set, uint32_t binding);
	void BindTextureToDescriptorSets(VkImageView textureImageView, VkSampler textureSampler, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t binding);
	//void BindTextureToDescriptorSetsCmd(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkImageView textureImageView, VkSampler textureSampler, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t set, uint32_t binding);

	// ~ general pipeline functions ~

	//#This is an error-tolerant function, all mesh will have the same amount of texture slot, but some may use less.
	//#You won't have any debug layer error with this, but the texture with higher slot number will remain the same for those who use less slots.
	VkDescriptorSetLayout GetLargestFrameDescriptorSetLayout() const;

	//#This is a conservative function, all mesh will have the same number of texture slot, but textures with higher slot number will be omitted.
	//#You will have a debug layer error when you want to bind more textures than the descriptor set layout allows.
	VkDescriptorSetLayout GetSmallestFrameDescriptorSetLayout() const;

	//return imageIndex if it can be accquired
	uint32_t WaitForFence();
	void BeginCommandBuffer(VkCommandBuffer commandBuffer);
	void EndCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CreateDescriptorSetLayout(
		VkDescriptorSetLayout& descriptorSetLayout, 
		uint32_t uboCount, 
		uint32_t uboBindingOffset, 
		uint32_t texCount, 
		uint32_t texBindingOffset);

	void CreateDescriptorSet(
		VkDescriptorSet& descriptorSet, 
		VkDescriptorPool descriptorPool, 
		VkDescriptorSetLayout descriptorSetLayout);

	void CreateDescriptorPool(
		VkDescriptorPool& descriptorPool, 
		uint32_t maxSets, 
		uint32_t uboCount, 
		uint32_t texCount);

	void CreateFramebuffer(
		VkFramebuffer& framebuffer, 
		const std::vector<VkImageView>& preResolveViews, 
		const std::vector<VkImageView>& depthViews,
		const std::vector<VkImageView>& colorViews,
		VkRenderPass renderPass, 
		uint32_t width, 
		uint32_t height);
	
	void CreateRenderPass(
		VkRenderPass& renderPass, 
		const std::vector<VkAttachmentDescription>& preResolveAttachments, 
		const std::vector<VkAttachmentDescription>& depthAttachments,
		const std::vector<VkAttachmentDescription>& colorAttachments);

	void CreatePipeline(
		VkPipeline& pipeline,
		VkPipelineLayout& pipelineLayout,
		VkRenderPass renderPass,
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayout,
		VkExtent2D extent,
		VkSampleCountFlagBits msaaSamples,
		Shader* pVertShader,
		Shader* pTesCtrlShader,
		Shader* pTesEvalShader,
		Shader* pGeomShader,
		Shader* pFragShader);

	void CreatePipeline(
		VkPipeline& pipeline,
		VkPipelineLayout& pipelineLayout,
		VkDescriptorSetLayout frameDescriptorSetLayout,
		const Pass& pass);

	void RecordCommand(
		glm::vec4 colorClear,
		glm::vec2 depthStencilClear,
		Pass& pass,
		VkCommandBuffer commandBuffer,
		VkPipeline pipeline,
		VkPipelineLayout pipelineLayout,
		VkRenderPass renderPassFallback,
		VkFramebuffer frameBufferFallback,
		VkExtent2D extentFallback);

	void RecordCommandNoEnd(
		glm::vec4 colorClear,
		glm::vec2 depthStencilClear,
		Pass& pass,
		VkCommandBuffer commandBuffer,
		VkPipeline pipeline,
		VkPipelineLayout pipelineLayout,
		VkRenderPass renderPassFallback,
		VkFramebuffer frameBufferFallback,
		VkExtent2D extentFallback);

	void RecordCommandEnd(VkCommandBuffer commandBuffer);

	// ~ window ~

	GLFWwindow* window;

	// ~ default pool and command buffers ~

	VkDescriptorPool defaultDescriptorPool;
	VkCommandPool defaultCommandPool;
	std::vector<VkCommandBuffer> defaultCommandBuffers;

	// ~ swap chain render pass, framebuffer and extent ~

	VkRenderPass swapChainRenderPass;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkExtent2D swapChainExtent;

	// ~ frame uniforms ~

	std::vector<Frame> frameVec;

	// ~ graphics pipeline resources ~
	
	VkPipeline graphicsPipeline;
	VkPipelineLayout graphicsPipelineLayout;

	// ~ defered pipeline resources ~

	VkPipeline offscreenPipeline;
	VkPipelineLayout offscreenPipelineLayout;

private:


	// ~ structures ~

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// ~ constants ~

	const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	// ~ member variables ~

	std::vector<Level*> pLevelVec;
	int width;
	int height;
	int framesInFlight;

	// ~ app ~

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	// ~ gpu device ~

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	// ~ gpu queue ~ 

	QueueFamilyIndices queueFamilyIndices;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// ~ swap chain resources ~

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	// ~ color buffer ~

	//msaa
	//By default we'll be using only one sample per pixel which is equivalent to no multisampling,
	//in which case the final image will remain unchanged. The exact maximum number of samples can be 
	//extracted from VkPhysicalDeviceProperties associated with our selected physical device.
	VkSampleCountFlagBits swapChainMsaaSamples;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	// ~ depth buffer ~

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	// ~ barriers ~

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame = 0;
	bool framebufferResized = false;

	// ~ clean up ~

	void CleanUpLevels();

	// ~ glfw resize ~

	//static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);//handle resize explicitly

	// ~ debug layer ~

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	bool CheckValidationLayerSupport();
	void SetupDebugMessenger();

	// ~ device ~

	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	std::vector<const char*> GetRequiredExtensions();

	// ~ swap chain ~

	void CreateSwapChain();
	void CleanUpSwapChain();//clean up all gpu resources related to swap chain
	void CreateImageViews();
	//void RecreateSwapChain();//handle resize
	void CreateSyncObjects();
	void CreateColorResources();//msaa
	void CreateDepthResources();
	void CreateFrameUniformBuffers();
	void CreateSwapChainRenderPass();
	void CreateSwapChainFramebuffers();
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	// ~ utility ~

	void CreateCommandPool(VkCommandPool& _commandPool);
	void CreateCommandBuffers(VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers);
	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
	void EndSingleTimeCommands(VkCommandBuffer& commandBuffer, VkCommandPool commandPool);//contains idle wait
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool HasStencilComponent(VkFormat format);
};
