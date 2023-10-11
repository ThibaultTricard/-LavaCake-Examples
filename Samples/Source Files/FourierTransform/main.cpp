#define LAVACAKE_WINDOW_MANAGER_GLFW
#include <LavaCake/Framework/Framework.h> 


using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;

#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="./";
#endif

int main() {
	ErrorCheck::printError(true, 2);

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(512, 512, "Fourier Transform", nullptr, nullptr);

	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Framework::Device* d = Framework::Device::getDevice();
	d->initDevices(1, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();

	GraphicQueue queue = d->getGraphicQueue(0);
	PresentationQueue presentQueue = d->getPresentQueue();

	ComputeQueue compute_queue = d->getComputeQueue(0);

	VkDevice logical = d->getLogicalDevice();
	VkExtent2D size = s->size();

	CommandBuffer commandBuffer;

	std::shared_ptr<Semaphore> s1 = std::make_shared<Semaphore>();


	//PostProcessQuad
	std::shared_ptr<Geometry::Mesh_t> quad = std::make_shared<Geometry::IndexedMesh<Geometry::TRIANGLE>>(Geometry::P3UV);

	quad->appendVertex({ -1.0,-1.0,0.0,0.0,0.0 });
	quad->appendVertex({ -1.0, 1.0,0.0,0.0,1.0 });
	quad->appendVertex({ 1.0, 1.0,0.0,1.0,1.0 });
	quad->appendVertex({ 1.0,-1.0,0.0,1.0,0.0 });

	quad->appendIndex(0);
	quad->appendIndex(1);
	quad->appendIndex(2);

	quad->appendIndex(2);
	quad->appendIndex(3);
	quad->appendIndex(0);

	VertexBuffer quad_vertex_buffer(queue, commandBuffer, std::vector<std::shared_ptr<Mesh_t>>({ quad }));

	//texture map
	Image  input = createTextureBuffer(queue, commandBuffer, prefix+"Data/Textures/mandrill.png", 4);


	
	std::vector<float> rawdata = std::vector<float>(input.width() * input.height() * uint32_t(2));
	Buffer output_pass1(queue, commandBuffer, rawdata, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);

	Buffer output_pass2(queue, commandBuffer, rawdata, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);

	UniformBuffer sizeBuffer;
	sizeBuffer.addVariable("width", input.width());
	sizeBuffer.addVariable("height", input.height());
	sizeBuffer.end();

	//pass1 
	Framework::ComputePipeline computePipeline1;

	ComputeShaderModule computeFourier1(prefix+"Data/Shaders/FourierTransform/fourier_pass1.comp.spv");
	computePipeline1.setComputeModule(computeFourier1);


	DescriptorSet firstPassSet;
	firstPassSet.addTextureBuffer(input, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	firstPassSet.addTexelBuffer(output_pass1, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	firstPassSet.addUniformBuffer(sizeBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 2);

	firstPassSet.compile();
	
	computePipeline1.setDescriptorLayout(firstPassSet.getLayout());
	computePipeline1.compile();

	//pass2
	Framework::ComputePipeline computePipeline2 = Framework::ComputePipeline();

	ComputeShaderModule computeFourier2(prefix+"Data/Shaders/FourierTransform/fourier_pass2.comp.spv");
	computePipeline2.setComputeModule(computeFourier2);

	DescriptorSet secondPassSet;

	
	secondPassSet.addTexelBuffer(output_pass1, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	secondPassSet.addTexelBuffer(output_pass2, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	secondPassSet.addUniformBuffer(sizeBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 2);
	secondPassSet.compile();

	computePipeline2.setDescriptorLayout(secondPassSet.getLayout());

	computePipeline2.compile();


	//renderPass
	Framework::RenderPass showPass;

	GraphicPipeline pipeline = GraphicPipeline (vec3f({ 0,0,0 }) , vec3f({ float(size.width),float(size.height),1.0f }) , vec2f({ 0,0 }) , vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule vertexShader(prefix+"Data/Shaders/FourierTransform/shader.vert.spv");
	FragmentShaderModule fragmentShader(prefix+"Data/Shaders/FourierTransform/shader.frag.spv");
	pipeline.setVertexModule(vertexShader);
	pipeline.setFragmentModule(fragmentShader);
	pipeline.setVerticesInfo(quad_vertex_buffer.getBindingDescriptions(), quad_vertex_buffer.getAttributeDescriptions(), quad_vertex_buffer.primitiveTopology());

	DescriptorSet renderSet;

	renderSet.addTexelBuffer(output_pass2, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	renderSet.addUniformBuffer(sizeBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	renderSet.compile();

	pipeline.setDescriptorLayout(renderSet.getLayout());

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	uint32_t subass_count = showPass.addSubPass(SA);
	pipeline.setSubPassNumber(subass_count);

	showPass.compile();

	pipeline.compile(showPass.getHandle(),SA.nbColor);

	Framework::FrameBuffer frameBuffers(s->size().width, s->size().height);

	showPass.prepareOutputFrameBuffer(queue, commandBuffer, frameBuffers);
	

	commandBuffer.wait();
	commandBuffer.resetFence();
	commandBuffer.beginRecord();

	sizeBuffer.update(commandBuffer);

	computePipeline1.bindPipeline(commandBuffer);
	computePipeline1.bindDescriptorSet(commandBuffer, firstPassSet);
	computePipeline1.compute(commandBuffer, input.width(), input.height(), 1);


	computePipeline2.bindPipeline(commandBuffer);
	computePipeline2.bindDescriptorSet(commandBuffer, secondPassSet);
	computePipeline2.compute(commandBuffer, input.width(), input.height(), 1);


	commandBuffer.endRecord();

	commandBuffer.submit(compute_queue, {}, { });



	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		const SwapChainImage& image = s->acquireImage();

		std::vector<waitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.wait();
		commandBuffer.resetFence();
		commandBuffer.beginRecord();


		showPass.setSwapChainImage(frameBuffers, image);

		showPass.begin(commandBuffer, frameBuffers, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		pipeline.bindPipeline(commandBuffer);
		
		pipeline.bindDescriptorSet(commandBuffer,renderSet);
		bindVertexBuffer(commandBuffer, *quad_vertex_buffer.getVertexBuffer());
		bindIndexBuffer(commandBuffer, *quad_vertex_buffer.getIndexBuffer());

		drawIndexed(commandBuffer, quad_vertex_buffer.getIndicesNumber());
		
		showPass.end(commandBuffer);

		commandBuffer.endRecord();

		commandBuffer.submit(queue, wait_semaphore_infos, { s1 });
		

		s->presentImage(presentQueue, image, { s1 });
	}

	d->waitForAllCommands();


  return 0;
}
