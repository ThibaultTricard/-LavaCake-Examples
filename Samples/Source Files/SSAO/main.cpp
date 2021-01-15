#include "Framework/Framework.h"
#include "AllHeaders.h"
#include "Common.h"
#include "VulkanDestroyer.h"
#include "Geometry/meshLoader.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;


int main() {


	Window w("LavaCake : SSAO", 1280, 720);
	//glfwSetWindowPos(w.m_window, 2000, 100);
	Mouse* mouse = Mouse::getMouse();
	Device* d = Device::getDevice();
	d->initDevices(0, 1, w.m_windowParams);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	Queue* graphicQueue = d->getGraphicQueue(0);
	CommandBuffer cmdBuf;
	cmdBuf.addSemaphore();
	VkExtent2D size = s->size();

	auto knight = Load3DModelFromObjFile("Data/Models/StrollingKnight.obj", true, true);
	Mesh_t* knightMesh = new IndexedMesh<TRIANGLE>(knight.first.first, knight.first.second, knight.second);

	VertexBuffer* vertexBuffer = new VertexBuffer({ knightMesh });
	vertexBuffer->allocate(graphicQueue, cmdBuf);


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Gbuffer																										//
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	//uniform buffer
	UniformBuffer* b = new UniformBuffer();
	mat4 proj = Helpers::PreparePerspectiveProjectionMatrix(static_cast<float>(size.width) / static_cast<float>(size.height),
		50.0f, 0.5f, 10.0f);
	mat4 modelView = Helpers::PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);
	b->addVariable("modelView", &modelView);
	b->addVariable("projection", &proj);
	b->end();


	RenderPass Gpass;
	GraphicPipeline* Gpipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	VertexShaderModule* Gvertex = new  VertexShaderModule("Data/Shaders/SSAO/Gbuffer.vert.spv");
	Gpipeline->setVextexShader(Gvertex);

	FragmentShaderModule* Gfragment = new  FragmentShaderModule("Data/Shaders/SSAO/Gbuffer.frag.spv");
	Gpipeline->setFragmentModule(Gfragment);


	Gpipeline->setVertices(vertexBuffer);
	Gpipeline->addUniformBuffer(b, VK_SHADER_STAGE_VERTEX_BIT, 0);

	
	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 3;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.storeDepth = true;
	SA.showOnScreenIndex = 1;

	Gpass.addSubPass({Gpipeline}, SA);
	Gpass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	Gpass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	Gpass.compile();
	

	FrameBuffer* G = new FrameBuffer(size.width, size.height);
	Gpass.prepareOutputFrameBuffer(*G);


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					SSAO																											//
	////////////////////////////////////////////////////////////////////////////////////////////////////////





	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Blur																											//
	////////////////////////////////////////////////////////////////////////////////////////////////////////





	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Lightig																										//
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	//PushConstant
	PushConstant* constant = new PushConstant();
	vec4f LigthPos = vec4f({ 0.f,4.f,0.7f,0.0 });
	constant->addVariable("LigthPos", &LigthPos);



	bool updateUniform = true;

	vec2d* lastMousePos = nullptr;

	vec2d polars = vec2d({ 0.0,0.0 });


	while (w.running()) {
		w.updateInput();

		VkDevice logical = d->getLogicalDevice();
		VkQueue& presentQueue = d->getPresentQueue()->getHandle();
		
		VkSwapchainKHR& swapchain = s->getHandle();

		SwapChainImage& swapChainImage = s->AcquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			swapChainImage.getSemaphore(),												// VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});

		if (mouse->leftButton) {
			if (lastMousePos == nullptr) {
				lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
			}
			polars[0] += float((mouse->position[0] - (*lastMousePos)[0]) / 512.0f) * 360;
			polars[1] -= float((mouse->position[1] - (*lastMousePos)[1]) / 512.0f) * 360;

			updateUniform = true;
			modelView = Helpers::Identity();

			modelView = modelView * Helpers::PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);

			modelView = modelView * Helpers::PrepareRotationMatrix(-float(polars[0]), vec3f({ 0 , 1, 0 }));
			modelView = modelView * Helpers::PrepareRotationMatrix(float(polars[1]), vec3f({ 1 , 0, 0 }));
			//std::cout << w.m_mouse.position[0] << std::endl;
			b->setVariable("modelView", &modelView);
			lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
		}
		else {
			lastMousePos = nullptr;
		}



		cmdBuf.wait(MAXUINT32);
		cmdBuf.resetFence();

		cmdBuf.beginRecord();

		if (updateUniform) {
			b->update(cmdBuf.getHandle());
			updateUniform = false;
		}


		Gpass.setSwapChainImage(*G, swapChainImage);

		Gpass.draw(cmdBuf.getHandle(), G->getHandle(), vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } });

		cmdBuf.endRecord();

		if (!SubmitCommandBuffersToQueue(graphicQueue->getHandle(), wait_semaphore_infos, { cmdBuf.getHandle() }, { cmdBuf.getSemaphore(0) }, cmdBuf.getFence())) {
			continue;
		}

		PresentInfo present_info = {
			swapchain,                                    // VkSwapchainKHR         Swapchain
			swapChainImage.getIndex()                     // uint32_t               ImageIndex
		};

		if (!PresentImage(presentQueue, { cmdBuf.getSemaphore(0) }, { present_info })) {
			continue;
		}

	}

	d->end();
}