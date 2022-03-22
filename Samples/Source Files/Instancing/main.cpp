#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h>

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;


#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="";
#endif

std::shared_ptr < GraphicPipeline > pipeline;
VertexBuffer* triangle_vertex_buffer;
std::vector<PushConstant* > constants(3);

RenderPass* createRenderPass(const Queue& queue, CommandBuffer& commandBuffer) {
	SwapChain* s = SwapChain::getSwapChain();
	VkExtent2D size = s->size();
	//We define the stride format we need for the mesh here 
	//each vertex is a 3D position followed by a RGB color
	



	VertexShaderModule vertexShader(prefix + "Data/Shaders/Instancing/shader.vert.spv");
	FragmentShaderModule fragmentShader(prefix + "Data/Shaders/Instancing/shader.frag.spv");

	pipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(triangle_vertex_buffer->getBindingDescriptions(), triangle_vertex_buffer->getAttributeDescriptions() ,triangle_vertex_buffer->primitiveTopology());
	pipeline->setPushContantInfo({ { constants[0]->size(), VK_SHADER_STAGE_VERTEX_BIT } });


	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	RenderPass* pass = new RenderPass();
	pass->addSubPass({ pipeline }, SA);
	pass->addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->compile();

	return pass;
	
}

bool g_resize = false;

void window_size_callback(GLFWwindow* window, int width, int height)
{
	g_resize = true;
}



int main() {


	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(512, 512, "Lavacake: Instancing", nullptr, nullptr);

	ErrorCheck::printError(true);
	GLFWSurfaceInitialisator surfaceInitialisator(window);

	glfwSetWindowSizeCallback(window, window_size_callback);
	
	Device* d = Device::getDevice();
	d->initDevices(0, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();

	VkExtent2D size = s->size();
	GraphicQueue queue = d->getGraphicQueue(0);
	PresentationQueue presentQueue = d->getPresentQueue();
	CommandBuffer  commandBuffer;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();


	vertexFormat format = vertexFormat({ POS3,COL3 });

	//we create a indexed triangle mesh with the desired format
	Mesh_t* triangle = new IndexedMesh<TRIANGLE>(format);

	//adding 3 vertices
	triangle->appendVertex({ -0.75f, 0.75f, 0.0f, 1.0f , 0.0f , 0.0f });
	triangle->appendVertex({ 0.75f,	0.75f , 0.0f, 0.0f , 1.0f	, 0.0f });
	triangle->appendVertex({ 0.0f , -0.75f, 0.0f, 0.0f , 0.0f	, 1.0f });


	// we link the 3 vertices to define a triangle
	triangle->appendIndex(0);
	triangle->appendIndex(1);
	triangle->appendIndex(2);


	auto model0 = PrepareTranslationMatrix(-0.3, -0.4, 0.0)  * PrepareScalingMatrix(0.3, 0.3, 1.0);
	auto model1 = PrepareTranslationMatrix(0.3, -0.5, 0.0) * PrepareScalingMatrix(0.3, 0.3, 1.0);
	auto model2 = PrepareTranslationMatrix(0.3, 0.5, 0.0) * PrepareScalingMatrix(0.3, 0.3, 1.0);

	constants[0] = new PushConstant();
	constants[0]->addVariable("model", model0);
	constants[1] = new PushConstant();
	constants[1]->addVariable("model", model1);
	constants[2] = new PushConstant();
	constants[2]->addVariable("model", model2);

	//creating an allocating a vertex buffer
	triangle_vertex_buffer = new VertexBuffer(queue, commandBuffer, { triangle });


	auto pass = createRenderPass(queue, commandBuffer);
	
	FrameBuffer* frameBuffers = new FrameBuffer(size.width, size.height);
	pass->prepareOutputFrameBuffer(queue, commandBuffer, *frameBuffers);


	pipeline->setVertices({
		{ triangle_vertex_buffer, {{constants[0],VK_SHADER_STAGE_VERTEX_BIT}} },
		{ triangle_vertex_buffer, {{constants[1],VK_SHADER_STAGE_VERTEX_BIT}} },
		{ triangle_vertex_buffer, {{constants[2],VK_SHADER_STAGE_VERTEX_BIT}} }
	});

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	
		VkDevice logical = d->getLogicalDevice();
		

		commandBuffer.wait(2000000000);
		commandBuffer.resetFence();

		if (g_resize) {
			d->waitForAllCommands();
			s->resize();
			size = s->size();
			delete pass;
			pass = createRenderPass(queue, commandBuffer);
			delete frameBuffers;
			frameBuffers = new FrameBuffer(size.width, size.height);
			pass->prepareOutputFrameBuffer(queue, commandBuffer, *frameBuffers);

			pipeline->setVertices({
				{ triangle_vertex_buffer, {{constants[0],VK_SHADER_STAGE_VERTEX_BIT}} },
				{ triangle_vertex_buffer, {{constants[1],VK_SHADER_STAGE_VERTEX_BIT}} },
				{ triangle_vertex_buffer, {{constants[2],VK_SHADER_STAGE_VERTEX_BIT}} }
			});
			g_resize = false;
		}

		const SwapChainImage& image = s->acquireImage();

		std::vector<waitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.beginRecord();

		
		pass->setSwapChainImage(*frameBuffers, image);

		pass->draw(commandBuffer, *frameBuffers, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		commandBuffer.endRecord();

		commandBuffer.submit(queue, wait_semaphore_infos, { semaphore });

		s->presentImage(presentQueue, image, { semaphore });
	}
	d->waitForAllCommands();
}
