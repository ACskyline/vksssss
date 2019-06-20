#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

#include "Scene.h"
#include "GlobalInclude.h"

class Scene;
class Shader;
class Pass;

class Renderer
{
public:
	Renderer(int _width, int _hight, int _framesInFlight);
	~Renderer();

	void AddScene(Scene* pScene);
	void InitWindow();
	void InitVulkan();
	void InitDescriptorPool(VkDescriptorPool& descriptorPool);
	void InitAssets(VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
	void MainLoop();

	// ~ get general vulkan resources ~

	VkDevice GetDevice() const;

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

	// ~ utility ~

	VkFormat FindDepthFormat();

	// ~ general pipeline functions ~

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
		const std::vector<VkImageView>& colorViews, 
		const std::vector<VkImageView>& depthViews, 
		VkRenderPass renderPass, 
		uint32_t width, 
		uint32_t height);

	void CreateRenderPass(
		VkRenderPass& renderPass, 
		const std::vector< VkAttachmentDescription>& colorAttachments, 
		const std::vector< VkAttachmentDescription>& depthAttachments);

	void CreatePipeline(
		VkPipeline& pipeline,
		VkPipelineLayout& pipelineLayout,
		VkRenderPass renderPass,
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayout,
		Shader* pVertShader,
		Shader* pTesCtrlShader,
		Shader* pTesEvalShader,
		Shader* pGeomShader,
		Shader* pFragShader);

	void RecordCommand(
		Pass& pass,
		VkCommandBuffer commandBuffer,
		VkPipeline pipeline,
		VkPipelineLayout pipelineLayout);

	void RecordCommandOverride(
		Pass& pass,
		VkCommandBuffer commandBuffer,
		VkPipeline pipeline,
		VkPipelineLayout pipelineLayout,
		VkRenderPass renderPass,
		VkFramebuffer frameBuffer,
		VkExtent2D extent);

	// ~ graphics pipeline functions ~

	VkPipeline& GetGraphicsPipelineRef();
	VkRenderPass GetGraphicsRenderPass() const;
	VkPipelineLayout& GetGraphicsPipelineLayoutRef();
	VkDescriptorSetLayout& GetGraphicsDescriptorSetLayoutRef(int slot);
	VkDescriptorSetLayout GetGraphicsDescriptorSetLayout(int slot) const;
	std::vector<VkDescriptorSetLayout>& GetGraphicsDescriptorSetLayoutsRef();
	const std::vector<VkDescriptorSetLayout>& GetGraphicsDescriptorSetLayouts() const;
	VkDescriptorPool GetGraphicsDescriptorPool() const;
	VkDescriptorPool& GetGraphicsDescriptorPoolRef();
	VkCommandPool GetGraphicsCommandPool();

private:
	// ~ graphics pipeline resources ~

	VkPipeline graphicsPipeline;
	VkRenderPass graphicsRenderPass;
	VkPipelineLayout graphicsPipelineLayout;
	std::vector<VkDescriptorSetLayout> graphicsDescriptorSetLayouts;
	VkDescriptorPool graphicsDescriptorPool;
	VkCommandPool graphicsCommandPool;
	std::vector<VkCommandBuffer> graphicsCommandBuffers;

	// ~ defered pipeline resources ~

	//...

	// ~ scene containers ~

	std::vector<Scene*> pSceneVec;

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

	int width;
	int height;
	int framesInFlight;

	// ~ app & window ~

	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	// ~ gpu device ~

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	// ~ gpu queue ~ 

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// ~ swap chain ~

	VkSwapchainKHR swapChain;
	VkExtent2D swapChainExtent;
	VkFormat swapChainImageFormat;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// ~ frame uniforms ~

	std::vector<VkBuffer> frameUniformBuffers;
	std::vector<VkDeviceMemory> frameUniformBufferMemorys;
	std::vector<FrameUniformBufferObject> frameUniformBufferObjects;
	std::vector<VkDescriptorSet> frameDescriptorSets;

	// ~ color buffer ~

	//msaa
	//By default we'll be using only one sample per pixel which is equivalent to no multisampling,
	//in which case the final image will remain unchanged. The exact maximum number of samples can be 
	//extracted from VkPhysicalDeviceProperties associated with our selected physical device.
	VkSampleCountFlagBits msaaSamples;// = VK_SAMPLE_COUNT_1_BIT;
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

	void CleanUp();
	void CleanUpScenes();

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
	VkSampleCountFlagBits GetMaxUsableSampleCount();
	void BindFrameUniformBuffers();
	void CreateFrameUniformBuffers();
	void UpdateFrameUniformBuffer(uint32_t currentFrame);
	void CreateFrameDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
	void CreateCommandPool(VkCommandPool& _commandPool);
	void CreateCommandBuffers(VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers);
	void CreateSwapChainRenderPass(VkRenderPass& renderPass);
	void CreateSwapChainFramebuffers(VkRenderPass renderPass);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	// ~ general command buffer function ~

	//return imageIndex if it can be accquired
	uint32_t BeginCommandBuffer(const std::vector<VkCommandBuffer>& commandBuffers);
	void EndCommandBuffer(const std::vector<VkCommandBuffer>& commandBuffers, uint32_t imageIndex);

	// ~ utility ~

	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
	void EndSingleTimeCommands(VkCommandBuffer& commandBuffer, VkCommandPool commandPool);//contains idle wait
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool HasStencilComponent(VkFormat format);
};
