#define LAVACAKE_WINDOW_MANAGER_GLFW
#include "Framework/Framework.h"

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

	Device* d = Device::getDevice();
	d->initDevices(1, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	Queue* queue = d->getGraphicQueue(0);

	PresentationQueue* presentQueue = d->getPresentQueue();
	CommandBuffer* cmbBuff = new CommandBuffer();
	cmbBuff->addSemaphore();

	ComputePipeline* jumpFloodPipeline = new ComputePipeline();
	VkExtent2D size = s->size();
	Buffer* seeds = new Buffer();
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

	seeds->allocate(queue, *cmbBuff, data, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_FORMAT_R32G32B32A32_SFLOAT);

	jumpFloodPipeline->addTexelBuffer(seeds, VK_SHADER_STAGE_COMPUTE_BIT, 0);

	ComputeShaderModule* compute = new ComputeShaderModule(prefix+"Data/Shaders/JumpFlooding/JumpFlooding.comp.spv");
	jumpFloodPipeline->setComputeModule(compute);
	UniformBuffer* passNumber = new UniformBuffer();
	passNumber->addVariable("dimention", LavaCake::vec2i({ 512,512 }));
	passNumber->addVariable("passNumber", (unsigned int)(0));
	passNumber->end();
	jumpFloodPipeline->addUniformBuffer(passNumber, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	jumpFloodPipeline->compile();



	//PostProcessQuad
	Mesh_t* quad = new IndexedMesh<TRIANGLE>(P3UV);

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

	

	VertexBuffer* quad_vertex_buffer = new VertexBuffer({ quad });
	quad_vertex_buffer->allocate(queue, *cmbBuff);

	//renderPass
	RenderPass* showPass = new RenderPass();

	GraphicPipeline* pipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule* vertexShader = new VertexShaderModule(prefix+"Data/Shaders/JumpFlooding/shader.vert.spv");
	FragmentShaderModule* fragmentShader = new FragmentShaderModule(prefix+"Data/Shaders/JumpFlooding/shader.frag.spv");
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());
	pipeline->setVertices({ quad_vertex_buffer });
	pipeline->addTexelBuffer(seeds, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	pipeline->addUniformBuffer(passNumber, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	showPass->addSubPass({ pipeline }, SA);

	showPass->compile();

	FrameBuffer* frameBuffer = new FrameBuffer(s->size().width, s->size().height);
	showPass->prepareOutputFrameBuffer(queue, *cmbBuff, *frameBuffer);

	std::vector<VkBufferMemoryBarrier> seed_memory_barriers;
	std::vector<VkBufferMemoryBarrier> print_memory_barriers;

	seed_memory_barriers.push_back({
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType    sType
					nullptr,                                    // const void       * pNext
					VK_ACCESS_SHADER_READ_BIT ,                // VkAccessFlags      srcAccessMask
					VK_ACCESS_SHADER_WRITE_BIT ,                // VkAccessFlags      dstAccessMask
					d->getGraphicQueue(0)->getIndex(),          // uint32_t           srcQueueFamilyIndex
					d->getComputeQueue(0)->getIndex(),          // uint32_t           dstQueueFamilyIndex
					seeds->getHandle(),													// VkBuffer           buffer
					0,                                          // VkDeviceSize       offset
					VK_WHOLE_SIZE                               // VkDeviceSize       size
		});

	print_memory_barriers.push_back({
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // VkStructureType    sType
					nullptr,                                    // const void       * pNext
					VK_ACCESS_SHADER_WRITE_BIT ,                // VkAccessFlags      srcAccessMask
					VK_ACCESS_SHADER_READ_BIT ,									// VkAccessFlags      dstAccessMask
					d->getComputeQueue(0)->getIndex(),          // uint32_t           srcQueueFamilyIndex
					d->getGraphicQueue(0)->getIndex(),          // uint32_t           dstQueueFamilyIndex
					seeds->getHandle(),													// VkBuffer           buffer
					0,                                          // VkDeviceSize       offset
					VK_WHOLE_SIZE                               // VkDeviceSize       size
		});

	int pass = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (pass != 0) {
			cmbBuff->wait(2000000000);
		}
		cmbBuff->resetFence();
		cmbBuff->beginRecord();
		passNumber->setVariable("passNumber", pass + 1);
		passNumber->update(*cmbBuff);
		jumpFloodPipeline->compute(*cmbBuff, 512, 512, 1);
		LavaCake::vkCmdPipelineBarrier(cmbBuff->getHandle(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, (uint32_t)print_memory_barriers.size(), print_memory_barriers.data(), 0, nullptr);
		cmbBuff->endRecord();
		cmbBuff->submit(queue, {}, {});
		
		pass++;
		SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		cmbBuff->wait(2000000000);
		cmbBuff->resetFence();
		cmbBuff->beginRecord();


		showPass->setSwapChainImage(*frameBuffer, image);


		showPass->draw(*cmbBuff, *frameBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });
		LavaCake::vkCmdPipelineBarrier(cmbBuff->getHandle(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, (uint32_t)seed_memory_barriers.size(), seed_memory_barriers.data(), 0, nullptr);

		cmbBuff->endRecord();

		cmbBuff->submit(queue, wait_semaphore_infos, { cmbBuff->getSemaphore(0) });
		

		s->presentImage(presentQueue, image, { cmbBuff->getSemaphore(0) });
#ifdef _WIN32
		Sleep(500);
#endif
	}
	d->waitForAllCommands();
	delete cmbBuff;
	delete jumpFloodPipeline;
	delete compute;
	delete seeds;
	delete showPass;
	delete vertexShader;
	delete fragmentShader;
	delete pipeline;
	delete frameBuffer;
}
