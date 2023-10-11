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
std::shared_ptr < VertexBuffer > triangle_vertex_buffer;
std::vector< std::shared_ptr< PushConstant>  > constants(3);

RenderPass* createRenderPass(const Queue& queue, CommandBuffer& commandBuffer) {
	
	
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
	std::shared_ptr<Mesh_t> triangle = std::make_shared<IndexedMesh<TRIANGLE>>(format);

	//adding 3 vertices
	triangle->appendVertex({ -0.75f, 0.75f, 0.0f, 1.0f , 0.0f , 0.0f });
	triangle->appendVertex({ 0.75f,	0.75f , 0.0f, 0.0f , 1.0f	, 0.0f });
	triangle->appendVertex({ 0.0f , -0.75f, 0.0f, 0.0f , 0.0f	, 1.0f });


	// we link the 3 vertices to define a triangle
	triangle->appendIndex(0);
	triangle->appendIndex(1);
	triangle->appendIndex(2);


	auto model0 = PrepareScalingMatrix(0.3f, 0.3f, 1.0f) * PrepareTranslationMatrix(-0.3f, -0.4f, 0.0f)  ;
	auto model1 = PrepareScalingMatrix(0.3f, 0.3f, 1.0f) * PrepareTranslationMatrix(0.3f, -0.5f, 0.0f) ;
	auto model2 = PrepareScalingMatrix(0.3f, 0.3f, 1.0f) * PrepareTranslationMatrix(0.3f, 0.5f, 0.0f) ;

	auto model3 = Identity();


	UniformBuffer buffer;
	buffer.addVariable("model0", transpose(model0));
	buffer.addVariable("model1", transpose(model1));
	buffer.addVariable("model2", transpose(model2));
	buffer.end();

	DescriptorSet set;
	set.addUniformBuffer(buffer, VK_SHADER_STAGE_VERTEX_BIT, 0);
	set.compile();


	//creating an allocating a vertex buffer
	triangle_vertex_buffer = std::make_shared<VertexBuffer>(queue, commandBuffer, std::vector<std::shared_ptr<Mesh_t>>{ triangle });

	//We define the stride format we need for the mesh here 
	//each vertex is a 3D position followed by a RGB color
	

	VertexShaderModule vertexShader(prefix + "Data/Shaders/Instancing/shader.vert.spv");
	FragmentShaderModule fragmentShader(prefix + "Data/Shaders/Instancing/shader.frag.spv");

	pipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(triangle_vertex_buffer->getBindingDescriptions(), triangle_vertex_buffer->getAttributeDescriptions() ,triangle_vertex_buffer->primitiveTopology());
	pipeline->setDescriptorLayout(set.getLayout());
	



	SubPassAttachments SA;
  	SA.addSwapChainImageAttachment(s->imageFormat());
  	SA.setDepthFormat(VK_FORMAT_D16_UNORM);


	RenderPass* pass = new RenderPass();
	pass->addSubPass(SA);
	pass->addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->compile();

	pipeline->compile(pass->getHandle(), SA);

	FrameBuffer* frameBuffers = new FrameBuffer(size.width, size.height);
	pass->prepareOutputFrameBuffer(queue, commandBuffer, *frameBuffers);



	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	
		VkDevice logical = d->getLogicalDevice();
		

		commandBuffer.wait();
		commandBuffer.resetFence();
		

		const SwapChainImage& image = s->acquireImage();

		std::vector<waitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.beginRecord();

		buffer.update(commandBuffer);
		
		pass->setSwapChainImage(*frameBuffers, image);

		pass->begin(commandBuffer, *frameBuffers, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		pipeline->bindPipeline(commandBuffer);
		
		pipeline->bindDescriptorSet(commandBuffer,set);
		bindVertexBuffer(commandBuffer, *triangle_vertex_buffer->getVertexBuffer());
		bindIndexBuffer(commandBuffer, *triangle_vertex_buffer->getIndexBuffer());

		drawIndexed(commandBuffer, triangle_vertex_buffer->getIndicesNumber(),0,3);

		pass->end(commandBuffer);

		commandBuffer.endRecord();

		commandBuffer.submit(queue, wait_semaphore_infos, { semaphore });

		s->presentImage(presentQueue, image, { semaphore });
	}
	commandBuffer.wait();
	commandBuffer.resetFence();
}
