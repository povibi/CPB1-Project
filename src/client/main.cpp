#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <vector>
bool WIREFRAME_MODE=false;
void err(int errnum, const char* err){
	std::cerr << errnum << " -- " << err << std::endl;
}
VkShaderModule loadShaderModuleFromFile(VkDevice device,std::string filename){
	std::ifstream file(filename,std::ios::ate | std::ios::binary);
	unsigned int filesize = file.tellg();
	file.seekg(0);
	std::vector<char> bytecode(filesize);
	file.read(bytecode.data(), filesize);
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytecode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
	VkShaderModule shaderModule;
	if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)==VK_SUCCESS){
		std::cout << "Loaded shader " << filename << std::endl;
		return shaderModule;
	}
	else{
		std::cerr << "Failed to load shader " << filename << std::endl;
		return NULL;
	}
}
int main(){
	glfwSetErrorCallback(err);
	if(!glfwInit()){
		std::cerr << "glfw did not initialize !" << std::endl;
		glfwTerminate();
		return -1;
	}
	if(glfwVulkanSupported() == GLFW_FALSE){
		std::cerr << "Vulkan Not Supported !" << std::endl;
		glfwTerminate();
		return -1;
	}

	GLFWwindow *window;

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

	window = glfwCreateWindow(800, 600, "test", NULL,NULL);

	VkInstance vkinstance;
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "test";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
	applicationInfo.pEngineName = "test";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 1, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;

	unsigned int count=0;
	const char** exts = glfwGetRequiredInstanceExtensions(&count);
	std::vector<const char*> extensionVector;
	for(unsigned int i=0;i<count;i++){
		extensionVector.push_back(exts[i]);
	}
	extensionVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	createInfo.enabledExtensionCount = count;
	createInfo.ppEnabledExtensionNames = extensionVector.data();
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	std::vector<const char*> selectedLayers;
	selectedLayers.push_back("VK_LAYER_KHRONOS_validation");
	createInfo.enabledLayerCount = selectedLayers.size();
	createInfo.ppEnabledLayerNames=selectedLayers.data();
	std::cout << "Available Layers:" << std::endl;
	for(unsigned int i=0;i<availableLayers.size();i++){
		std::cout << "\t- " << availableLayers[i].layerName << std::endl;
	}
	std::cout << "Selected Layers:" << std::endl;
	for(unsigned int i=0;i<selectedLayers.size();i++){
		std::cout << "\t- " << selectedLayers[i] << std::endl;
	}
	std::cout << "Loaded extensions:" << std::endl;
	for(unsigned int i=0;i<extensionVector.size();i++){
		std::cout << "\t- " << extensionVector[i] << std::endl;
	}
	
	vkCreateInstance(&createInfo, NULL,&vkinstance);
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkinstance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkinstance, &deviceCount, devices.data());
	std::cout << "Available devices:" << std::endl;
	std::string deviceName;
	for (long unsigned int i=0;i<devices.size();i++) {
		VkPhysicalDeviceProperties vkp;
		vkGetPhysicalDeviceProperties(devices[i],&vkp);
		std::cout << "\t- " << vkp.deviceName << std::endl;
		if(physicalDevice==NULL or
				vkp.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
			physicalDevice=devices[i];
			deviceName=vkp.deviceName;
		}
	}
	std::cout << "Selected device: " << std::endl
	<< "\t- " << deviceName << std::endl;

	uint32_t graphicsFamilyID;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
	for (long unsigned int i=0;i<queueFamilies.size();i++) {
		if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
			graphicsFamilyID = i;
			std::cout << "Graphics queue family ID:" << std::endl
			<< "\t- " << i << std::endl;
		}
	}
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsFamilyID;
	queueCreateInfo.queueCount = 1;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid=true;
	VkDeviceCreateInfo devCreateInfo = {};
	devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	devCreateInfo.queueCreateInfoCount = 1;
	devCreateInfo.pEnabledFeatures = &deviceFeatures;

	std::vector<const char*> logicalExtensionVector;
	logicalExtensionVector.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	devCreateInfo.enabledExtensionCount = logicalExtensionVector.size();
	devCreateInfo.ppEnabledExtensionNames = logicalExtensionVector.data();
	VkDevice device = nullptr;
	if (vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device) != VK_SUCCESS) {
		std::cerr << "Failed to create logical device." << std::endl ;
		glfwTerminate();
	}
	VkQueue graphicsQueue;
	vkGetDeviceQueue(device, graphicsFamilyID, 0, &graphicsQueue);
	VkSurfaceKHR surface;
	if ( glfwCreateWindowSurface(vkinstance, window, nullptr, &surface) != VK_SUCCESS){
		std::cerr << "Cannot create window surface " << std::endl ;
		glfwTerminate();
	}
	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsFamilyID, surface, &presentSupport);
	if(presentSupport){
		std::cout << "Queue supports presentation." << std::endl;
	}
	else{
		std::cerr << "This program is not designed to support presentation on a different queue than graphics." << std::endl;
		glfwTerminate();
	}
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
	}
	
	VkSurfaceFormatKHR format = formats[0];
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(unsigned int i=0;i<presentModeCount;i++){
		if(presentModes[i]==VK_PRESENT_MODE_MAILBOX_KHR){
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			std::cout << "Surface supports triple buffering." << std::endl;
		}
	}
	VkExtent2D extent = {800,600};
	uint32_t imageCount=capabilities.minImageCount+1;
	if(capabilities.minImageCount == capabilities.maxImageCount){
		imageCount-=1;
	}
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = format.format;
	swapchainCreateInfo.imageColorSpace = format.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain;
	if(vkCreateSwapchainKHR(device,&swapchainCreateInfo,nullptr,&swapchain) != VK_SUCCESS){
		std::cerr << "Could not create swap chain." << std::endl;
		glfwTerminate();
	}
	else{
		std::cout << "Swapchain successfully created." << std::endl;
	}
	std::vector<VkImage> swapChainImages;
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapChainImages.data());
	VkFormat swapChainImageFormat = format.format;
	std::vector<VkImageView> ImageViews;
	ImageViews.resize(imageCount);
	for(unsigned int i=0;i<imageCount;i++){
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapChainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapChainImageFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		if(vkCreateImageView(device,&imageViewCreateInfo,nullptr,&ImageViews[i])!=VK_SUCCESS){
			std::cerr << "Could not create Image View." << i << std::endl;
			glfwTerminate();
		}

	}

	auto fragmentShaderModule = loadShaderModuleFromFile(device,"frag.sprv");
	auto vertexShaderModule = loadShaderModuleFromFile(device,"vert.sprv");

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaders[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;

	rasterizer.rasterizerDiscardEnable = VK_FALSE;

	if(WIREFRAME_MODE){
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	}
	else{
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	}
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	
	VkPipelineMultisampleStateCreateInfo multisampler = {};
	multisampler.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampler.sampleShadingEnable = VK_FALSE;
	multisampler.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional


	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
	    std::cerr << "Could not create pipeline layout." << std::endl;
	    glfwTerminate();
	}
	else{
	    std::cout << "Pipeline layout successfully created." << std::endl;
	}
	
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We do not need the stencil buffer for now
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		std::cerr << "Could not create renderpass." << std::endl;
		glfwTerminate();
	}
	else{
	    std::cout << "Renderpass successfully created." << std::endl;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount=2;
	pipelineCreateInfo.pStages=shaders;
	pipelineCreateInfo.pVertexInputState=&vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState=&inputAssembly;
	pipelineCreateInfo.pViewportState=&viewportStateInfo;
	pipelineCreateInfo.pRasterizationState=&rasterizer;
	pipelineCreateInfo.pMultisampleState=&multisampler;
	pipelineCreateInfo.pColorBlendState=&colorBlending;
	pipelineCreateInfo.layout=pipelineLayout;
	pipelineCreateInfo.renderPass=renderPass;
	pipelineCreateInfo.subpass=0;

	VkPipeline graphicsPipeline; // FINALLY the graphics pipeline
	if (vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		std::cerr << "Could not create the graphics pipeline." << std::endl;
		glfwTerminate();
	}
	else{
	    std::cout << "Graphics Pipeline successfully created." << std::endl;
	}
	
	std::vector<VkFramebuffer> Framebuffers;
	Framebuffers.resize(imageCount);
	for(unsigned int i=0;i<imageCount;i++){
		VkImageView attachments[] = {
			ImageViews[i]
	   	};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = extent.width;
		framebufferCreateInfo.height = extent.height;
		framebufferCreateInfo.layers = 1;
		
		if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &Framebuffers[i]) != VK_SUCCESS) {
			std::cerr << "Could not create framebuffer "<< i << std::endl;
			glfwTerminate();
		}
		else{
		    std::cout << "Framebuffer " << i << " successfully created." << std::endl;
		}
	}





	while(!glfwWindowShouldClose(window)){
		glfwPollEvents();

	}


	for(unsigned int i=0;i<imageCount;i++){
		vkDestroyFramebuffer(device,Framebuffers[i],nullptr);
	}
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	for(unsigned int i=0;i<imageCount;i++){
		vkDestroyImageView(device,ImageViews[i],nullptr);
	}
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(vkinstance,surface, nullptr);
	vkDestroyInstance(vkinstance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();

}
