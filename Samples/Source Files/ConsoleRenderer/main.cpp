#include "Framework/Framework.h"
#include "Geometry/meshLoader.h"

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
	uint32_t nbFrames = 1;
	ErrorCheck::PrintError(true);

	Window w("LavaCake HelloWorld", 512, 512);

	Mouse* mouse = Mouse::getMouse();

	Device* d = Device::getDevice();
	d->initDevices(0, 1, w.m_windowParams);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();

	Queue* queue = d->getGraphicQueue(0);
	PresentationQueue* present_queue = d->getPresentQueue();
	std::vector<CommandBuffer> commandBuffer = std::vector<CommandBuffer>(nbFrames);
	VkExtent2D size = s->size();


	CommandBuffer allocBuff;

	for (int i = 0; i < nbFrames; i++) {
		commandBuffer[i].addSemaphore();
	}

	vec3f camera = vec3f({0.0f,0.0f,4.0f});

	//knot mesh
	std::pair<std::vector<float>, Geometry::vertexFormat > knot = Geometry::Load3DModelFromObjFile(prefix+"Data/Models/knot.obj", true, false, false , true);
	Geometry::Mesh_t* knot_mesh = new Geometry::Mesh<Geometry::TRIANGLE>(knot.first, knot.second);


	//plane mesh
	std::pair<std::vector<float>, Geometry::vertexFormat > plane = Geometry::Load3DModelFromObjFile(prefix+"Data/Models/plane.obj", true, false, false, false);
	Geometry::Mesh_t* plane_mesh = new Geometry::Mesh<Geometry::TRIANGLE>(plane.first, plane.second);

	


	VertexBuffer* scene_vertex_buffer = new VertexBuffer({ plane_mesh, knot_mesh  });
	scene_vertex_buffer->allocate(queue, allocBuff);

	VertexBuffer* plane_buffer = new VertexBuffer({ plane_mesh });
	plane_buffer->allocate(queue, allocBuff);


	//PostProcessQuad
	Geometry::Mesh_t* quad = new Geometry::IndexedMesh<Geometry::TRIANGLE>(Geometry::P3UV);
	
	quad->appendVertex({ -1.0,-1.0,0.0,0.0,0.0 });
	quad->appendVertex({ -1.0, 1.0,0.0,0.0,1.0 });
	quad->appendVertex({  1.0, 1.0,0.0,1.0,1.0 });
	quad->appendVertex({  1.0,-1.0,0.0,1.0,0.0 });

	quad->appendIndex(0);
	quad->appendIndex(1);
	quad->appendIndex(2);

	quad->appendIndex(2);
	quad->appendIndex(3);
	quad->appendIndex(0);              
	
	VertexBuffer* quad_vertex_buffer = new VertexBuffer({ quad });
	quad_vertex_buffer->allocate(queue, commandBuffer[0]);

	//uniform buffer
	uint32_t shadowsize = 64;

	UniformBuffer* b = new UniformBuffer();
	mat4 proj = PreparePerspectiveProjectionMatrix(static_cast<float>(shadowsize) / static_cast<float>(shadowsize),
		50.0f, 0.5f, 10.0f);
	mat4 modelView = mat4 ({ 0.9981f, -0.0450f, 0.0412f, 0.0000f, 0.0000f, 0.6756f, 0.7373f, 0.0000f, -0.0610f, -0.7359f, 0.6743f, 0.0000f, -0.0000f, -0.0000f, -4.0000f, 1.0000f });
	mat4 lightView = mat4({ 1.0f,0.0f,0.0f,0.0f,0.0f,0.173648223f ,0.984807730f,0.0f,0.0f, -0.984807730f, 0.173648223f ,0.0f,0.0f,0.0f,-3.99999976f ,1.0f });
	b->addVariable("ligthView", lightView);
	b->addVariable("modelView", modelView);
	b->addVariable("projection", proj);
	b->end();

	//PushConstant
	PushConstant* constant = new PushConstant();
	vec4f LigthPos = vec4f({ 0.f,4.f,0.7f,0.0 });
	constant->addVariable("LigthPos", LigthPos);


	//DepthBuffer
	FrameBuffer* shadow_map_buffer = new FrameBuffer(shadowsize, shadowsize);

	//frameBuffer
	FrameBuffer* scene_buffer = new FrameBuffer(shadowsize, shadowsize);
	
	// Shadow pass
	RenderPass shadowMapPass = RenderPass();
	GraphicPipeline* shadowPipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(shadowsize),float(shadowsize),1.0f }), vec2f({ 0,0 }), vec2f({ float(shadowsize),float(shadowsize) }));

	VertexShaderModule* shadowVertex = new VertexShaderModule(prefix+"Data/Shaders/ConsoleRenderer/shadow.vert.spv");
	shadowPipeline->setVertexModule(shadowVertex);

	shadowPipeline->setVerticesInfo(scene_vertex_buffer->getBindingDescriptions(), scene_vertex_buffer->getAttributeDescriptions(), scene_vertex_buffer->primitiveTopology());

	shadowPipeline->setVertices({ scene_vertex_buffer });
	shadowPipeline->addUniformBuffer(b, VK_SHADER_STAGE_VERTEX_BIT, 0);

	SubpassAttachment SA;
	SA.nbColor = 0;
	SA.useDepth = true;
	SA.storeDepth = true;

	shadowMapPass.addSubPass({ shadowPipeline }, SA);
	shadowMapPass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	shadowMapPass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	shadowMapPass.compile();

	shadowMapPass.prepareOutputFrameBuffer(*shadow_map_buffer);

	//Render Pass
	RenderPass renderPass = RenderPass();
	GraphicPipeline* renderPipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(shadowsize),float(shadowsize),1.0f }), vec2f({ 0,0 }), vec2f({ float(shadowsize),float(shadowsize) }));
	VertexShaderModule* renderVertex = new VertexShaderModule(prefix+"Data/Shaders/ConsoleRenderer/scene.vert.spv");
	renderPipeline->setVertexModule(renderVertex);

	FragmentShaderModule* renderFrag = new FragmentShaderModule(prefix+"Data/Shaders/ConsoleRenderer/scene.frag.spv");
	renderPipeline->setFragmentModule(renderFrag);

	
	renderPipeline->addPushContant(constant, VK_SHADER_STAGE_VERTEX_BIT);
	renderPipeline->setVerticesInfo(scene_vertex_buffer->getBindingDescriptions(), scene_vertex_buffer->getAttributeDescriptions(), scene_vertex_buffer->primitiveTopology());
	renderPipeline->setVertices({ scene_vertex_buffer });
	renderPipeline->addUniformBuffer(b, VK_SHADER_STAGE_VERTEX_BIT, 0);

	renderPipeline->addFrameBuffer(shadow_map_buffer, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	SubpassAttachment SA2;
	SA2.nbColor = 1;
	SA2.useDepth = true;
	SA2.storeColor = true;

	renderPass.addSubPass({ renderPipeline }, SA2);
	//renderPass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	//renderPass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	renderPass.compile();

	renderPass.prepareOutputFrameBuffer(*scene_buffer);

	//Console Render pass
	RenderPass consolePass = RenderPass();
	GraphicPipeline* consolePipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule* consoleVertex = new VertexShaderModule(prefix+"Data/Shaders/ConsoleRenderer/console.vert.spv");
	consolePipeline->setVertexModule(consoleVertex);

	FragmentShaderModule* consoleFrag = new FragmentShaderModule(prefix+"Data/Shaders/ConsoleRenderer/console.frag.spv");
	consolePipeline->setFragmentModule(consoleFrag);

	consolePipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());

	consolePipeline->setVertices({ quad_vertex_buffer });

	consolePipeline->addFrameBuffer(scene_buffer, VK_SHADER_STAGE_FRAGMENT_BIT, 0);


	SubpassAttachment SA3;
	SA3.showOnScreen = true;
	SA3.nbColor = 1;
	SA3.storeColor = true;
	SA3.useDepth = true;
	SA3.showOnScreenIndex = 0;

	consolePass.addSubPass({ consolePipeline }, SA3);
	consolePass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	consolePass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	consolePass.compile();


	std::vector<FrameBuffer*> frameBuffers;
	for (int i = 0; i < nbFrames; i++) {
		frameBuffers.push_back(new FrameBuffer(s->size().width, s->size().height));
		consolePass.prepareOutputFrameBuffer(*frameBuffers[i]);
	}


	bool updateUniformBuffer = true;
	uint32_t f = 0;

	vec2d* lastMousePos = nullptr;

	vec2d polars = vec2d({ 0.0,0.0 });
	while (w.running()){
		w.updateInput();
		f++;
		f = f % nbFrames;
		
		VkDevice logical = d->getLogicalDevice();
		VkSwapchainKHR& swapchain = s->getHandle();
		
		SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});

		
		int state = glfwGetKey(w.m_window, GLFW_KEY_SPACE);
    if (state == GLFW_PRESS) {
			d->waitForAllCommands();
			consolePass.reloadShaders();
		}


		if (mouse->leftButton) {
			if (lastMousePos == nullptr) {
				lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
			}
			polars[0] += float((mouse->position[0] - (*lastMousePos)[0]) / 512.0f) * 360;
			polars[1] -= float((mouse->position[1] - (*lastMousePos)[1]) / 512.0f) * 360;

			updateUniformBuffer = true;
			modelView = Identity();

			modelView = modelView * PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);

			modelView = modelView * PrepareRotationMatrix(-float(polars[0]), vec3f({ 0 , 1, 0 }));
			modelView = modelView * PrepareRotationMatrix(float(polars[1]), vec3f({ 1 , 0, 0 }));
			//std::cout << w.m_mouse.position[0] << std::endl;
			b->setVariable("modelView", modelView);
			lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
		}
		else {
			lastMousePos = nullptr;
		}

	

		commandBuffer[f].wait(UINT32_MAX);
		commandBuffer[f].resetFence();
		commandBuffer[f].beginRecord();


		if (updateUniformBuffer) {
			b->update(commandBuffer[f]);
			updateUniformBuffer = false;
		}

		shadowMapPass.draw(commandBuffer[f], *shadow_map_buffer, vec2u({ 0,0 }), vec2u({shadowsize, shadowsize }));



		renderPass.draw(commandBuffer[f], *scene_buffer, vec2u({ 0,0 }), scene_buffer->size(), { { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0 } });


		consolePass.setSwapChainImage(*frameBuffers[f], image);
		consolePass.draw(commandBuffer[f], *frameBuffers[f], vec2u({ 0,0 }), vec2u({size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });


		commandBuffer[f].endRecord();

		commandBuffer[f].submit(queue, wait_semaphore_infos, { commandBuffer[f].getSemaphore(0) });
	

    s->presentImage(present_queue, image, { commandBuffer[f].getSemaphore(0) });
		

	}
	d->end();
}

