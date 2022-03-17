
#define LAVACAKE_WINDOW_MANAGER_GLFW
#include "Framework/Framework.h"

#include "RayTracing/RayTracingPipeline.h"
#include "RayTracing/BottomLevelAS.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;
using namespace LavaCake::RayTracing;


float lerp(float a, float b, float f)
{
	return a + f * (b - a);
} 
 
int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(512, 512, "LavaCake: Raytracing Cornell Box", nullptr, nullptr);

	ErrorCheck::printError(true, 5);

	GLFWSurfaceInitialisator surfaceInitialisator(window);

	
	

	Device* d = Device::getDevice();
	d->enableRaytracing(false);
	d->initDevices(0, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	VkExtent2D size = s->size();
	const Queue& queue = d->getGraphicQueue(0);
	const PresentationQueue& presentQueue = d->getPresentQueue();
	CommandBuffer  commandBuffer;
	commandBuffer.addSemaphore();


	//We define the stride format we need for the mesh here 
	vertexFormat format = vertexFormat({ POS3, F1 });


	//we create a indexed triangle mesh with the desired format
	
	std::array<std::array<float, 8>,4> materials;

	//light material
	materials[0] = std::array<float,8>({ 0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,0.0f, 0.0f });
	//white material
	materials[1] = std::array<float, 8>({ 1.0f,.824f,0.549f,0.0f,0.0f,0.0f,0.0f, 0.0f });
	//green material
	materials[2] =  std::array<float, 8>({ 0.0f,0.4f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f });
	//red material
	materials[3] = std::array<float, 8>({ 0.7f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f });

	UniformBuffer materialBuffer;
	materialBuffer.addVariable("material",materials);
	materialBuffer.end();


	std::array<vec4f,64> samples;
	srand(time(NULL));
	for (unsigned int i = 0; i < 64; i++)
	{
		vec3f noise({
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX });

		noise = Normalize(noise) ;

		samples[i] = vec4f({ noise[0], noise[1], noise[2],0.0 });
	}

	UniformBuffer sampleBuffer;
	sampleBuffer.addVariable("sample", samples);
	sampleBuffer.end();
	
	//547.8

	float ligthY = 547.8f;

	std::array<vec4f,64> lightSample;
	srand(time(NULL));
	for (unsigned int i = 0; i < 64; i++)
	{
		vec2f uv({
			(float)rand() / (float)RAND_MAX,
			(float)rand() / (float)RAND_MAX, });


		float x = 213.0 + (343.0 - 213.0) * uv[0];
		float y = ligthY;
		float z = 227.0 + (332.0 - 227.0) * uv[1];

		lightSample[i] = (vec4f({ x, y, z,0.0 }));
	}


	UniformBuffer lightSampleBuffer;
	lightSampleBuffer.addVariable("sample", lightSample);
	lightSampleBuffer.end();

	Mesh_t* light = new IndexedMesh<TRIANGLE>(format);

	

	

	light->appendVertex({ 343.0, ligthY, 227.0, 0.0f });
	light->appendVertex({ 343.0, ligthY, 332.0, 0.0f });
	light->appendVertex({ 213.0, ligthY, 332.0, 0.0f });
	light->appendVertex({ 213.0, ligthY, 227.0, 0.0f });

	light->appendIndex(0);
	light->appendIndex(1);
	light->appendIndex(2);

	light->appendIndex(2);
	light->appendIndex(3);
	light->appendIndex(0);

	Mesh_t* floor = new IndexedMesh<TRIANGLE>(format);
	floor->appendVertex({ 552.8f,  0.0f, 0.0f, 1.0f });
	floor->appendVertex({ 0.0f,		 0.0f, 0.0f, 1.0f });
	floor->appendVertex({ 0.0f,		 0.0f, 559.2f, 1.0f });
	floor->appendVertex({ 549.6f,  0.0f, 559.2f, 1.0f });

	floor->appendIndex(0);
	floor->appendIndex(1);
	floor->appendIndex(2);

	floor->appendIndex(2);
	floor->appendIndex(3);
	floor->appendIndex(0);


	Mesh_t* ceiling = new IndexedMesh<TRIANGLE>(format);

	ceiling->appendVertex({ 556.0f,  548.8f, 0.0, 1.0f });
	ceiling->appendVertex({ 556.0f,  548.8f, 559.2f, 1.0f });
	ceiling->appendVertex({ 0.0f,    548.8f, 559.2f, 1.0f });
	ceiling->appendVertex({ 0.0f,    548.8f,   0.0, 1.0f });

	ceiling->appendIndex(0);
	ceiling->appendIndex(1);
	ceiling->appendIndex(2);

	ceiling->appendIndex(2);
	ceiling->appendIndex(3);
	ceiling->appendIndex(0);


	Mesh_t* backWall = new IndexedMesh<TRIANGLE>(format);

	backWall->appendVertex({ 549.6f,  0.0f,			559.2f, 1.0f });
	backWall->appendVertex({ 0.0f,    0.0f,			559.2f, 1.0f });
	backWall->appendVertex({ 0.0f,    548.8f,		559.2f, 1.0f });
	backWall->appendVertex({ 556.0f,  548.8f,   559.2f, 1.0f });

	backWall->appendIndex(0);
	backWall->appendIndex(1);
	backWall->appendIndex(2);

	backWall->appendIndex(2);
	backWall->appendIndex(3);
	backWall->appendIndex(0);


	Mesh_t* rightWall = new IndexedMesh<TRIANGLE>(format);

	rightWall->appendVertex({ 0.0f,    0.0f,			559.2f, 2.0f });
	rightWall->appendVertex({ 0.0f,    0.0f,				0.0f, 2.0f });
	rightWall->appendVertex({ 0.0f,    548.8f,			0.0f, 2.0f });
	rightWall->appendVertex({ 0.0f,    548.8f,    559.2f, 2.0f });

	rightWall->appendIndex(0);
	rightWall->appendIndex(1);
	rightWall->appendIndex(2);

	rightWall->appendIndex(2);
	rightWall->appendIndex(3);
	rightWall->appendIndex(0);


	Mesh_t* leftWall = new IndexedMesh<TRIANGLE>(format);

	leftWall->appendVertex({ 552.8f,    0.0f,			   0.0f, 3.0f });
	leftWall->appendVertex({ 549.6f,    0.0f,			 559.2f, 3.0f });
	leftWall->appendVertex({ 556.0f,    548.8f,		 559.2f, 3.0f });
	leftWall->appendVertex({ 556.0f,    548.8f,      0.0f, 3.0f });

	leftWall->appendIndex(0);
	leftWall->appendIndex(1);
	leftWall->appendIndex(2);

	leftWall->appendIndex(2);
	leftWall->appendIndex(3);
	leftWall->appendIndex(0);


	Mesh_t* box1 = new IndexedMesh<TRIANGLE>(format);

	box1->appendVertex({ 130.0, 165.0,  65.0, 1.0f });
	box1->appendVertex({  82.0, 165.0, 225.0, 1.0f });
	box1->appendVertex({ 240.0, 165.0, 272.0, 1.0f });
	box1->appendVertex({ 290.0, 165.0, 114.0, 1.0f });

	box1->appendVertex({ 130.0,   0.0,  65.0, 1.0f });
	box1->appendVertex({ 82.0,		0.0, 225.0, 1.0f });
	box1->appendVertex({ 240.0,		0.0, 272.0, 1.0f });
	box1->appendVertex({ 290.0,		0.0, 114.0, 1.0f });

	//face up
	box1->appendIndex(0);
	box1->appendIndex(1);
	box1->appendIndex(2);

	box1->appendIndex(2);
	box1->appendIndex(3);
	box1->appendIndex(0);

	//face left

	box1->appendIndex(4);
	box1->appendIndex(5);
	box1->appendIndex(0);


	box1->appendIndex(0);
	box1->appendIndex(5);
	box1->appendIndex(1);

	//face back
	box1->appendIndex(1);
	box1->appendIndex(6);
	box1->appendIndex(2);

	box1->appendIndex(6);
	box1->appendIndex(1);
	box1->appendIndex(5);

	//face right

	box1->appendIndex(3);
	box1->appendIndex(2);
	box1->appendIndex(7);

	box1->appendIndex(7);
	box1->appendIndex(2);
	box1->appendIndex(6);


	//face forward
	box1->appendIndex(0);
	box1->appendIndex(3);
	box1->appendIndex(4);

	box1->appendIndex(4);
	box1->appendIndex(3);
	box1->appendIndex(7);
	


	Mesh_t* box2 = new IndexedMesh<TRIANGLE>(format);

	box2->appendVertex({ 423.0, 330.0, 247.0, 1.0f });
	box2->appendVertex({ 265.0, 330.0, 296.0, 1.0f });
	box2->appendVertex({ 314.0, 330.0, 456.0, 1.0f });
	box2->appendVertex({ 472.0, 330.0, 406.0, 1.0f });

	box2->appendVertex({ 423.0, 0.0, 247.0, 1.0f });
	box2->appendVertex({ 265.0, 0.0, 296.0, 1.0f });
	box2->appendVertex({ 314.0, 0.0, 456.0, 1.0f });
	box2->appendVertex({ 472.0, 0.0, 406.0, 1.0f });

	//face up
	box2->appendIndex(0);
	box2->appendIndex(1);
	box2->appendIndex(2);

	box2->appendIndex(2);
	box2->appendIndex(3);
	box2->appendIndex(0);

	//face left

	box2->appendIndex(4);
	box2->appendIndex(5);
	box2->appendIndex(0);


	box2->appendIndex(0);
	box2->appendIndex(5);
	box2->appendIndex(1);

	//face back
	box2->appendIndex(1);
	box2->appendIndex(6);
	box2->appendIndex(2);

	box2->appendIndex(6);
	box2->appendIndex(1);
	box2->appendIndex(5);

	//face right

	box2->appendIndex(3);
	box2->appendIndex(2);
	box2->appendIndex(7);

	box2->appendIndex(7);
	box2->appendIndex(2);
	box2->appendIndex(6);


	//face forward
	box2->appendIndex(0);
	box2->appendIndex(3);
	box2->appendIndex(4);

	box2->appendIndex(4);
	box2->appendIndex(3);
	box2->appendIndex(7);

	//creating an allocating a vertex buffer
	std::shared_ptr<VertexBuffer> triangle_vertex_buffer = std::make_shared<VertexBuffer>(
		queue, 
		commandBuffer,
		std::vector<Mesh_t*>{ light,ceiling, leftWall, rightWall, floor, backWall,box1,box2 }, 
		(uint32_t)0, VK_VERTEX_INPUT_RATE_VERTEX, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

	VkTransformMatrixKHR transformMatrixKHR = { 1,0,0,0,
																						0,1,0,0,
																						0,0,1,0 };

	VkTransformMatrixKHR transformMatrixKHR2= { 1,0,0,0,
																						0,1,0,0,
																						0,0,1,0 };


	std::shared_ptr<TransformBuffer> transform = std::make_shared<TransformBuffer>(queue, commandBuffer, transformMatrixKHR2);

	BottomLevelAccelerationStructure blAS;
	blAS.addVertexBuffer(triangle_vertex_buffer, transform, true);
	blAS.allocate(queue, commandBuffer);

	TopLevelAccelerationStructure tlAS;

	tlAS.addInstance(&blAS, transformMatrixKHR, 0, 0);
	tlAS.alloctate(queue, commandBuffer, false);
	//we create a indexed triangle mesh with the desired format

	Image output = createStorageImage(queue, commandBuffer, size.width, size.height, 1, s->imageFormat());

	UniformBuffer proj;

	proj.addVariable("CameraPos", vec4f({ 278, 273, -800,0 }));
	proj.addVariable("Direction", vec4f({ 0, 0, 1,0 }));
	proj.addVariable("horizontal", vec4f({ 1, 0, 0,0 }));
	proj.addVariable("Up", vec3f({ 0, 1, 0}));
	proj.addVariable("focale", 1.4f);
	proj.addVariable("width", 0.025f);
	proj.addVariable("height", 0.025f);

	proj.end();


	RayGenShaderModule rayGenShaderModule("Data/Shaders/CornellBox/raygen.rgen.spv");
	MissShaderModule  missShaderModule("Data/Shaders/CornellBox/miss.rmiss.spv");
	ClosestHitShaderModule closesHitShaderModule("Data/Shaders/CornellBox/closesthit.rchit.spv");

	RayTracingPipeline rtPipeline(vec2u({ size.width, size.height }));
	rtPipeline.addAccelerationStructure(tlAS, VK_SHADER_STAGE_RAYGEN_BIT_KHR| VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0);
	rtPipeline.addStorageImage(output, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
	rtPipeline.addUniformBuffer(proj, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2);
	rtPipeline.addBuffer(*triangle_vertex_buffer->getVertexBuffer(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3);
	rtPipeline.addBuffer(*triangle_vertex_buffer->getIndexBuffer(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4);
	rtPipeline.addUniformBuffer(materialBuffer, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 5);
	rtPipeline.addUniformBuffer(sampleBuffer, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 6);
	rtPipeline.addUniformBuffer(lightSampleBuffer, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 7);
	rtPipeline.addRayGenModule(rayGenShaderModule);
	rtPipeline.addMissModule(missShaderModule);

	rtPipeline.startHitGroup();
	rtPipeline.setClosestHitModule(closesHitShaderModule);
	rtPipeline.endHitGroup();

	rtPipeline.setMaxRecursion(10);

	rtPipeline.compile(queue, commandBuffer);


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



	VertexBuffer* quad_vertex_buffer = new VertexBuffer(queue, commandBuffer, { quad });

	UniformBuffer passNumber;
	passNumber.addVariable("dimention", LavaCake::vec2u({ size.width, size.height }));
	passNumber.end();



	//renderPass
	RenderPass* showPass = new RenderPass();

	std::shared_ptr<GraphicPipeline> pipeline = std::make_shared<GraphicPipeline> (vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule vertexShader("Data/Shaders/CornellBox/shader.vert.spv");
	FragmentShaderModule fragmentShader ("Data/Shaders/CornellBox/shader.frag.spv");
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());
	pipeline->setVertices({ quad_vertex_buffer });
	pipeline->addStorageImage(output, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
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
	showPass->prepareOutputFrameBuffer(queue, commandBuffer, *frameBuffer);

	std::vector<VkBufferMemoryBarrier> seed_memory_barriers;
	std::vector<VkBufferMemoryBarrier> print_memory_barriers;

	bool first = true;


	commandBuffer.resetFence();
	commandBuffer.beginRecord();


	if (first) {
		materialBuffer.update(commandBuffer);
		sampleBuffer.update(commandBuffer);
		lightSampleBuffer.update(commandBuffer);
		proj.update(commandBuffer);
	}

	rtPipeline.trace(commandBuffer);
	commandBuffer.endRecord();

	commandBuffer.submit(queue, {}, {});

	commandBuffer.wait(UINT64_MAX);
	commandBuffer.resetFence();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		VkDevice logical = d->getLogicalDevice();
		
		if (first) {
			
			first = false;
		}
		

		const SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer.wait();
		commandBuffer.resetFence();
		commandBuffer.beginRecord();
		passNumber.update(commandBuffer);
		showPass->setSwapChainImage(*frameBuffer, image);


		showPass->draw(commandBuffer, *frameBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		
		commandBuffer.endRecord();

		commandBuffer.submit(queue, wait_semaphore_infos, { commandBuffer.getSemaphore(0) });


		s->presentImage(presentQueue, image, { commandBuffer.getSemaphore(0) });
		
	}
	d->waitForAllCommands();
}
