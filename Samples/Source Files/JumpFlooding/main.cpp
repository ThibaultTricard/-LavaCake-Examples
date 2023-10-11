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

int main() {

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(512, 512, "Lavacake: JumpFlooding", nullptr, nullptr);

	GLFWSurfaceInitialisator surfaceInitialisator(window);

	ErrorCheck::printError(true, 8);

	Device* d = Device::getDevice();
	d->initDevices(1, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	GraphicQueue graphicQueue = d->getGraphicQueue(0);
	ComputeQueue computeQueue = d->getComputeQueue(0);
	PresentationQueue presentQueue = d->getPresentQueue();
	CommandBuffer cmbBuff;
	std::shared_ptr<Semaphore> semaphore = std::make_shared<Semaphore>();

	ComputePipeline* jumpFloodPipeline = new ComputePipeline();
	VkExtent2D size = s->size();
	std::vector<float> data(512 * 512 * 4);

	for (uint32_t i = 0; i < data.size(); i++) {
		data[i] = -512.0f;
	}

	int nseeds = 200;

	//setting up seeds
	for (int i = 0; i < nseeds; i++) {
		int x = (int)(float(std::rand()) / float(RAND_MAX) * 512);
		int y = (int)(float(std::rand()) / float(RAND_MAX) * 512);

		data[(x + y * 512) * 4] = (float)x;
		data[(x + y * 512) * 4 + 1] = (float)y;
		data[(x + y * 512) * 4 + 2] = float(i)/ nseeds;
		data[(x + y * 512) * 4 + 3] = 0.0f;
	}

	Buffer seeds(graphicQueue, cmbBuff, data, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_FORMAT_R32G32B32A32_SFLOAT);


	std::shared_ptr<DescriptorSet> jumpfloodingSet = std::make_shared<DescriptorSet>();

	jumpfloodingSet->addTexelBuffer(seeds, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	UniformBuffer passNumber;
	passNumber.addVariable("dimention", LavaCake::vec2i({ 512,512 }));
	passNumber.addVariable("passNumber", (unsigned int)(0));
	passNumber.end();
	jumpfloodingSet->addUniformBuffer(passNumber, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);


	jumpfloodingSet->compile();

	ComputeShaderModule compute (prefix+"Data/Shaders/JumpFlooding/JumpFlooding.comp.spv");
	jumpFloodPipeline->setComputeModule(compute);
	jumpFloodPipeline->setDescriptorLayout(jumpfloodingSet->getLayout());
	jumpFloodPipeline->compile();



	//PostProcessQuad
	std::shared_ptr<Mesh_t> quad = std::make_shared<IndexedMesh<TRIANGLE>>(P3UV);

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

	

	std::shared_ptr<VertexBuffer> quad_vertex_buffer = std::make_shared<VertexBuffer>(graphicQueue, cmbBuff,std::vector<std::shared_ptr<Mesh_t>>{ quad });

	//renderPass
	RenderPass* showPass = new RenderPass();

	std::shared_ptr < GraphicPipeline > pipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule vertexShader(prefix+"Data/Shaders/JumpFlooding/shader.vert.spv");
	FragmentShaderModule fragmentShader(prefix+"Data/Shaders/JumpFlooding/shader.frag.spv");
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());
	pipeline->setDescriptorLayout(jumpfloodingSet->getLayout());

	SubPassAttachments SA;
  	SA.addSwapChainImageAttachment(s->imageFormat());
 

	showPass->addSubPass(SA);

	showPass->compile();


	pipeline->compile(showPass->getHandle(),SA);

	FrameBuffer frameBuffer(s->size().width, s->size().height);
	showPass->prepareOutputFrameBuffer(graphicQueue, cmbBuff, frameBuffer);

	std::vector<VkBufferMemoryBarrier> seed_memory_barriers;
	std::vector<VkBufferMemoryBarrier> print_memory_barriers;

	seed_memory_barriers.push_back({
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType    sType
					nullptr,                                    // const void       * pNext
					VK_ACCESS_SHADER_READ_BIT ,									// VkAccessFlags      srcAccessMask
					VK_ACCESS_SHADER_WRITE_BIT ,							  // VkAccessFlags      dstAccessMask
					graphicQueue.getIndex(),								    // uint32_t           srcQueueFamilyIndex
					computeQueue.getIndex(),                    // uint32_t           dstQueueFamilyIndex
					seeds.getHandle(),													// VkBuffer           buffer
					0,                                          // VkDeviceSize       offset
					VK_WHOLE_SIZE                               // VkDeviceSize       size
		});

	print_memory_barriers.push_back({
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType    sType
					nullptr,                                    // const void       * pNext
					VK_ACCESS_SHADER_WRITE_BIT ,                // VkAccessFlags      srcAccessMask
					VK_ACCESS_SHADER_READ_BIT ,									// VkAccessFlags      dstAccessMask
					computeQueue.getIndex(),                    // uint32_t           srcQueueFamilyIndex
					graphicQueue.getIndex(),                    // uint32_t           dstQueueFamilyIndex
					seeds.getHandle(),													// VkBuffer           buffer
					0,                                          // VkDeviceSize       offset
					VK_WHOLE_SIZE                               // VkDeviceSize       size
		});

	int pass = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (pass != 0) {
			cmbBuff.wait();
		}
		cmbBuff.resetFence();
		cmbBuff.beginRecord();
		passNumber.setVariable("passNumber", pass + 1);
		passNumber.update(cmbBuff);
		
		jumpFloodPipeline->bindPipeline(cmbBuff);
		jumpFloodPipeline->bindDescriptorSet(cmbBuff, *jumpfloodingSet);
		jumpFloodPipeline->compute(cmbBuff, 512, 512, 1);
		LavaCake::vkCmdPipelineBarrier(cmbBuff.getHandle(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, (uint32_t)print_memory_barriers.size(), print_memory_barriers.data(), 0, nullptr);
		
		cmbBuff.endRecord();
		cmbBuff.submit(computeQueue, {}, {});
		
		pass++;
		const SwapChainImage& image = s->acquireImage();

		std::vector<waitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
		});


		cmbBuff.wait();
		cmbBuff.resetFence();
		cmbBuff.beginRecord();
		

		showPass->setSwapChainImage(frameBuffer, image);
		cmbBuff.beginRecord();
		showPass->begin(cmbBuff, 
			frameBuffer, 
			vec2u({ 0,0 }), 
			vec2u({ size.width, size.height }), 
			{ { 0.1f, 0.2f, 0.3f, 1.0f } });

		pipeline->bindPipeline(cmbBuff);

		bindVertexBuffer(cmbBuff, *quad_vertex_buffer->getVertexBuffer());
		bindIndexBuffer(cmbBuff, *quad_vertex_buffer->getIndexBuffer());

		pipeline->bindDescriptorSet(cmbBuff, *jumpfloodingSet);

		drawIndexed(cmbBuff, quad_vertex_buffer->getIndicesNumber());
		
		showPass->end(cmbBuff);
		cmbBuff.endRecord();
		cmbBuff.submit(graphicQueue, wait_semaphore_infos, { semaphore });

		s->presentImage(presentQueue, image, { semaphore });


		
#ifdef _WIN32
		Sleep(500);
#endif
	}
	//d->waitForAllCommands();
}
