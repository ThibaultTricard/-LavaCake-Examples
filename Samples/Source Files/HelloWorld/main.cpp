#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h> 

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;

#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="../";
#endif

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(512, 512, "LavaCake HelloWorld", nullptr, nullptr);

	ErrorCheck::printError(true, 5);
	
	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Device* device = Device::getDevice();
	device->initDevices(0, 1, surfaceInitialisator);

	SwapChain* swapChain = SwapChain::getSwapChain();
	swapChain->init();

	CommandBuffer  commandBuffer;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	GraphicQueue graphicQueue = device->getGraphicQueue(0);
	PresentationQueue presentQueue = device->getPresentQueue();

	vertexFormat format ({ POS3,COL3 });

	//we create a indexed triangle mesh with the desired format
	std::shared_ptr<Mesh_t> triangle = std::make_shared<IndexedMesh<TRIANGLE>>(format);

	//adding 3 vertices
	triangle->appendVertex({ -0.75f, 0.75f, 0.0f, 1.0f , 0.0f , 0.0f });
	triangle->appendVertex({ 0.75f,	0.75f , 0.0f, 0.0f , 1.0f , 0.0f });
	triangle->appendVertex({ 0.0f , -0.75f, 0.0f, 0.0f , 0.0f , 1.0f });

	// we link the 3 vertices to define a triangle
	triangle->appendIndex(0);
	triangle->appendIndex(1);
	triangle->appendIndex(2);

	//creating an allocating a vertex buffer
	VertexBuffer triangle_vertex_buffer (
		graphicQueue,
		commandBuffer, 
		std::vector<std::shared_ptr<Mesh_t>>({ triangle }) 
	);

	VertexShaderModule vertexShader(prefix + "Data/Shaders/helloworld/shader.vert.spv");
	FragmentShaderModule fragmentShader(prefix + "Data/Shaders/helloworld/shader.frag.spv");

	VkExtent2D size = swapChain->size();

	GraphicPipeline graphicPipeline(
		vec3f({ 0,0,0 }), 
		vec3f({ float(size.width),float(size.height),1.0f }), 
		vec2f({ 0,0 }), 
		vec2f({ float(size.width),float(size.height) })
	);

	graphicPipeline.setVertexModule(vertexShader);
	graphicPipeline.setFragmentModule(fragmentShader);
	//graphicPipeline->setVertices({ triangle_vertex_buffer });
	graphicPipeline.setVerticesInfo(
		triangle_vertex_buffer.getBindingDescriptions(), 
		triangle_vertex_buffer.getAttributeDescriptions(), 
		triangle_vertex_buffer.primitiveTopology());
	

	graphicPipeline.setDescriptorLayout(generateEmptyLayout());

	RenderPass renderPass;

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.showOnScreenIndex = 0;

	uint32_t passNumber = renderPass.addSubPass(SA);
	graphicPipeline.setSubPassNumber(passNumber);

	renderPass.compile();

	graphicPipeline.compile(renderPass.getHandle(),SA.nbColor);

	FrameBuffer frameBuffer =  FrameBuffer(size.width, size.height);
	renderPass.prepareOutputFrameBuffer(graphicQueue, commandBuffer, frameBuffer);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		commandBuffer.wait();
		commandBuffer.resetFence();

		const SwapChainImage& image = swapChain->acquireImage();
		std::vector<waitSemaphoreInfo> waitSemaphoreInfos = {};
		waitSemaphoreInfos.push_back({
			image.getSemaphore(),                           // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT	  // VkPipelineStageFlags   WaitingStage
		});

		renderPass.setSwapChainImage(frameBuffer, image);
		commandBuffer.beginRecord();
		renderPass.begin(commandBuffer, 
			frameBuffer, 
			vec2u({ 0,0 }), 
			vec2u({ size.width, size.height }), 
			{ { 0.1f, 0.2f, 0.3f, 1.0f } });

		graphicPipeline.bindPipeline(commandBuffer);

		bindVertexBuffer(commandBuffer, *triangle_vertex_buffer.getVertexBuffer());
		bindIndexBuffer(commandBuffer, *triangle_vertex_buffer.getIndexBuffer());

		drawIndexed(commandBuffer, triangle_vertex_buffer.getIndicesNumber());
		
		renderPass.end(commandBuffer);
		commandBuffer.endRecord();
		commandBuffer.submit(graphicQueue, waitSemaphoreInfos, { semaphore });

		swapChain->presentImage(presentQueue, image, { semaphore });
	}
	device->waitForAllCommands();
}
