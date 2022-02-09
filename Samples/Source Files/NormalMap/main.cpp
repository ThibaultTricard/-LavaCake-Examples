#include "Framework/Framework.h"
#include "AllHeaders.h"
#include "Common.h"
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
	int nbFrames = 3;
	ErrorCheck::PrintError(true);
	Window w("LavaCake : Bump Mapping", 500, 500);
	Mouse* mouse = Mouse::getMouse();

	Device* d = Device::getDevice();
	d->initDevices(0, 1, w.m_windowParams);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	VkExtent2D size = s->size();
	Queue* queue = d->getGraphicQueue(0);
	auto presentQueue = d->getPresentQueue();
	std::vector<CommandBuffer> commandBuffer = std::vector<CommandBuffer>(nbFrames);
	for (int i = 0; i < nbFrames; i++) {
		commandBuffer[i].addSemaphore();
	}

	//Normal map
	Image* normalMap = createTextureBuffer(queue, commandBuffer[0],prefix+"Data/Textures/normal_map.png", 4);

	//vertex buffer
	//knot mesh
	std::pair<std::vector<float>, vertexFormat > ice = Load3DModelFromObjFile(prefix+"Data/Models/ice.obj", true,true, true, true);
	Geometry::Mesh_t* ice_mesh = new Geometry::Mesh<Geometry::TRIANGLE>(ice.first, ice.second);



	VertexBuffer* v = new VertexBuffer({ ice_mesh });
	v->allocate(queue, commandBuffer[0]);

	//uniform buffer
	UniformBuffer* b = new UniformBuffer();
	mat4 proj = PreparePerspectiveProjectionMatrix(static_cast<float>(size.width) / static_cast<float>(size.height),
		50.0f, 0.5f, 10.0f);
	mat4 modelView = PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);
	b->addVariable("modelView", modelView);
	b->addVariable("projection", proj);
	b->end();


	//PushConstant
	PushConstant* constant = new PushConstant();
	vec4f LigthPos = vec4f({ 0.f,4.f,0.7f,0.0 });
	constant->addVariable("LigthPos", LigthPos);

	// Render pass
	RenderPass pass = RenderPass();
	GraphicPipeline* pipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	VertexShaderModule* vertex = new VertexShaderModule(prefix+"Data/Shaders/NormalMap/shader.vert.spv");
	pipeline->setVertexModule(vertex);


	FragmentShaderModule* frag = new FragmentShaderModule(prefix+"Data/Shaders/NormalMap/shader.frag.spv");
	pipeline->setFragmentModule(frag);

	pipeline->setVerticesInfo(v->getBindingDescriptions(), v->getAttributeDescriptions(), v->primitiveTopology());

	pipeline->setVertices({ { v, { {constant,VK_SHADER_STAGE_FRAGMENT_BIT} } } });
	pipeline->addUniformBuffer(b, VK_SHADER_STAGE_VERTEX_BIT, 0);
	pipeline->addTextureBuffer(normalMap, VK_SHADER_STAGE_FRAGMENT_BIT,1);
	pipeline->setPushContantInfo({ {constant->size(), VK_SHADER_STAGE_FRAGMENT_BIT} });
	//pipeline->addPushContant(constant, VK_SHADER_STAGE_FRAGMENT_BIT);

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	pass.addSubPass({ pipeline }, SA);
	pass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass.compile();

	std::vector<FrameBuffer*> frameBuffers;
	for (int i = 0; i < nbFrames; i++) {
		frameBuffers.push_back(new FrameBuffer(s->size().width, s->size().height));
		pass.prepareOutputFrameBuffer(queue, commandBuffer[0], *frameBuffers[i]);
	}

	bool updateUniformBuffer = true;
	int f = 0;

	vec2d* lastMousePos = nullptr;

	vec2d polars = vec2d({ 0.0,0.0 });

	while (w.running()) {
		w.updateInput();
		f++;
		f = f % nbFrames;

		VkDevice logical = d->getLogicalDevice();
		VkQueue& present_queue = d->getPresentQueue()->getHandle();
		SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),											        // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
		});

		VkSwapchainKHR& swapchain = s->getHandle();
		VkExtent2D size = s->size();

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


		commandBuffer[f].wait(2000000000);
		commandBuffer[f].resetFence();
		commandBuffer[f].beginRecord();



		if (updateUniformBuffer) {
			b->update(commandBuffer[f]);
			updateUniformBuffer = false;
		}


		pass.setSwapChainImage(*frameBuffers[f], image);


		pass.draw(commandBuffer[f], *frameBuffers[f], vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });


		commandBuffer[f].endRecord();

		commandBuffer[f].submit(queue, wait_semaphore_infos, { commandBuffer[f].getSemaphore(0) });
	

		s->presentImage(presentQueue, image, { commandBuffer[f].getSemaphore(0) });
	}
	d->end();
}

