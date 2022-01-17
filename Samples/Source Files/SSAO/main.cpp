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

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

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

	ImGuiWrapper* gui = new ImGuiWrapper();
	gui->initGui(&w, d->getGraphicQueue(0), &cmdBuf);
	auto knight = Load3DModelFromObjFile(prefix+"Data/Models/StrollingKnight.obj", true, true);
	Mesh_t* knightMesh = new IndexedMesh<TRIANGLE>(knight.first.first, knight.first.second, knight.second);

	VertexBuffer* vertexBuffer = new VertexBuffer({ knightMesh });
	vertexBuffer->allocate(graphicQueue, cmdBuf);


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Gbuffer																										//
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	//uniform buffer
	UniformBuffer* b = new UniformBuffer();
	mat4 proj = PreparePerspectiveProjectionMatrix(static_cast<float>(size.width) / static_cast<float>(size.height),
		50.0f, 0.01f, 5.0f);
	mat4 modelView = PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);
	b->addVariable("modelView", modelView);
	b->addVariable("projection", proj);
	b->end();


	RenderPass Gpass(VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT);
	GraphicPipeline* Gpipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	VertexShaderModule* Gvertex = new  VertexShaderModule(prefix+"Data/Shaders/SSAO/Gbuffer.vert.spv");
	Gpipeline->setVertexModule(Gvertex);

	FragmentShaderModule* Gfragment = new  FragmentShaderModule(prefix+"Data/Shaders/SSAO/Gbuffer.frag.spv");
	Gpipeline->setFragmentModule(Gfragment);

	Gpipeline->setVerticesInfo(vertexBuffer->getBindingDescriptions(), vertexBuffer->getAttributeDescriptions(), vertexBuffer->primitiveTopology());
	Gpipeline->setVertices({ vertexBuffer });
	Gpipeline->addUniformBuffer(b, VK_SHADER_STAGE_VERTEX_BIT, 0);

	
	SubpassAttachment SA;
	SA.showOnScreen = false;
	SA.nbColor = 3;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.storeDepth = true;
	SA.showOnScreenIndex = 2;

	Gpass.addSubPass({Gpipeline}, SA);
	Gpass.addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	Gpass.addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	Gpass.compile();
	

	FrameBuffer* G = new FrameBuffer(size.width, size.height);
	Gpass.prepareOutputFrameBuffer(graphicQueue, cmdBuf, *G);


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					SSAO																											//
	////////////////////////////////////////////////////////////////////////////////////////////////////////



	Mesh_t* quadMesh = new IndexedMesh<TRIANGLE>(P3UV);

	quadMesh->appendVertex({ -1, -1, 0, 0, 0 });
	quadMesh->appendVertex({ -1, 1, 0, 0, 1 });
	quadMesh->appendVertex({ 1, 1, 0, 1, 1 });
	quadMesh->appendVertex({ 1, -1, 0, 1, 0 });

	quadMesh->appendIndex(0);
	quadMesh->appendIndex(1);
	quadMesh->appendIndex(2);

	quadMesh->appendIndex(2);
	quadMesh->appendIndex(3);
	quadMesh->appendIndex(0);

	VertexBuffer* quadBuffer = new VertexBuffer({quadMesh});
	quadBuffer->allocate(graphicQueue, cmdBuf);


	UniformBuffer* SSAOuni = new UniformBuffer();

	std::array<vec4f, 64> samples;
	srand(time(NULL));
	for (unsigned int i = 0; i < 64; i++)
	{
		vec3f noise({
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX });

		noise = Normalize(noise) * ((float)rand() / (float)RAND_MAX);
		
		float scale = (float)i / 64.0;
		scale = lerp(0.1f, 1.0f, scale * scale);
		noise =noise * scale;

		samples[i] = vec4f({ noise[0], noise[1], noise[2],0.0 });
	}

	std::array<vec4f, 16> randomVectors;
	srand(time(NULL));
	for (unsigned int i = 0; i < 16; i++)
	{
		vec3f noise({
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f,
			(float)rand() / (float)RAND_MAX * 2.0f - 1.0f });

		noise = Normalize(noise) ;


		randomVectors[i] =  vec4f({ noise[0], noise[1], noise[2],0.0 });
	}


	float radius = 0.5;

	SSAOuni->addVariable("samples", samples);
	SSAOuni->addVariable("projection", proj);
	SSAOuni->addVariable("radius", radius);
	SSAOuni->addVariable("seed", randomVectors);
	SSAOuni->end();


	RenderPass* SSAORenderPass = new RenderPass(VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

	GraphicPipeline* SSAOpipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	VertexShaderModule* SSAOvertex = new VertexShaderModule(prefix+"Data/Shaders/SSAO/SSAO.vert.spv");
	FragmentShaderModule* SSAOfragment = new FragmentShaderModule(prefix+"Data/Shaders/SSAO/SSAO.frag.spv");

	SSAOpipeline->setVerticesInfo(quadBuffer->getBindingDescriptions(), quadBuffer->getAttributeDescriptions(), quadBuffer->primitiveTopology());
	SSAOpipeline->setVertices({quadBuffer});

	SSAOpipeline->addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0);
	SSAOpipeline->addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);
	SSAOpipeline->addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 3);

	SSAOpipeline->addUniformBuffer(SSAOuni, VK_SHADER_STAGE_FRAGMENT_BIT, 4);

	SSAOpipeline->setVertexModule(SSAOvertex);
	SSAOpipeline->setFragmentModule(SSAOfragment);
	//SSAOpipeline->SetCullMode();

	SubpassAttachment SSAOattachment;
	SSAOattachment.showOnScreen = false;
	SSAOattachment.nbColor = 1;
	SSAOattachment.storeColor = true;
	SSAOattachment.useDepth = true;
	SSAOattachment.storeDepth = false;
	SSAOattachment.showOnScreenIndex = 0;

	SSAORenderPass->addSubPass({ SSAOpipeline }, SSAOattachment);
	SSAORenderPass->compile();


	FrameBuffer* SSAO = new FrameBuffer(size.width, size.height);
	SSAORenderPass->prepareOutputFrameBuffer(graphicQueue, cmdBuf, *SSAO);



	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Blur																											//
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	RenderPass* Blurpass = new RenderPass(VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

	GraphicPipeline blurPipeline (vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));


	blurPipeline.setVerticesInfo(quadBuffer->getBindingDescriptions(), quadBuffer->getAttributeDescriptions(), quadBuffer->primitiveTopology());
	blurPipeline.setVertices({ quadBuffer });

	VertexShaderModule* blurVert = new VertexShaderModule(prefix+"Data/Shaders/SSAO/blur.vert.spv");
	FragmentShaderModule* blurFrag = new FragmentShaderModule(prefix+"Data/Shaders/SSAO/blur.frag.spv");

	blurPipeline.setVertexModule(blurVert);
	blurPipeline.setFragmentModule(blurFrag);

	blurPipeline.addFrameBuffer(SSAO,VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0);

	SubpassAttachment Blurattachment;
	Blurattachment.showOnScreen = false;
	Blurattachment.nbColor = 1;
	Blurattachment.storeColor = true;
	Blurattachment.useDepth = true;
	Blurattachment.storeDepth = false;
	Blurattachment.showOnScreenIndex = 0;

	Blurpass->addSubPass({ &blurPipeline }, Blurattachment);
	Blurpass->compile();

	FrameBuffer* BlurBuffer = new FrameBuffer(size.width, size.height);
	Blurpass->prepareOutputFrameBuffer(graphicQueue, cmdBuf,*BlurBuffer);

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																					Lighting																									//
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	UniformBuffer lightbuffer;
	lightbuffer.addVariable("position", vec4f({ 0.0,0.0,0.0,0.0 }));
	lightbuffer.addVariable("color", vec4f({ 1.0,1.0,1.0,0.0 }));
	lightbuffer.end();



	RenderPass* Lightingpass = new RenderPass();

	GraphicPipeline lightingPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	lightingPipeline.setVertices({ quadBuffer });
	lightingPipeline.setVerticesInfo(quadBuffer->getBindingDescriptions(), quadBuffer->getAttributeDescriptions(), quadBuffer->primitiveTopology());

	VertexShaderModule* lightingVert = new VertexShaderModule(prefix+"Data/Shaders/SSAO/lighting.vert.spv");
	FragmentShaderModule* lightingFrag = new FragmentShaderModule(prefix+"Data/Shaders/SSAO/lighting.frag.spv");

	lightingPipeline.setVertexModule(lightingVert);
	lightingPipeline.setFragmentModule(lightingFrag);

	lightingPipeline.addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 0);
	lightingPipeline.addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);
	lightingPipeline.addFrameBuffer(G, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 2);
	lightingPipeline.addFrameBuffer(BlurBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 0);
	lightingPipeline.addUniformBuffer(&lightbuffer, VK_SHADER_STAGE_FRAGMENT_BIT, 4);

	SubpassAttachment lightingattachment;
	lightingattachment.showOnScreen = true;
	lightingattachment.nbColor = 1;
	lightingattachment.storeColor = true;
	lightingattachment.useDepth = true;
	lightingattachment.storeDepth = false;
	lightingattachment.showOnScreenIndex = 0;

	Lightingpass->addSubPass({ &lightingPipeline, gui->getPipeline() }, lightingattachment);
	Lightingpass->compile();

	FrameBuffer* lightingBuffer = new FrameBuffer(size.width, size.height);
	Lightingpass->prepareOutputFrameBuffer(graphicQueue, cmdBuf,*lightingBuffer);



	//PushConstant
	PushConstant* constant = new PushConstant();
	vec4f LigthPos = vec4f({ 0.f,4.f,0.7f,0.0 });
	constant->addVariable("LigthPos", LigthPos);



	bool updateUniform = true;

	vec2d* lastMousePos = nullptr;

	vec2d polars = vec2d({ 0.0,0.0 });

	float red = 1.0, green = 1.0, blue = 1.0;
	float x = 0, y = 0, z = 0;

	while (w.running()) {
		w.updateInput();

		VkDevice logical = d->getLogicalDevice();
		auto presentQueue = d->getPresentQueue();
		
		VkSwapchainKHR& swapchain = s->getHandle();

		SwapChainImage& swapChainImage = s->acquireImage();

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
			modelView = Identity();

			modelView = modelView * PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);

			modelView = modelView * PrepareRotationMatrix(polars[0], vec3f({ 0 , 1, 0 }));
			modelView = modelView * PrepareRotationMatrix(polars[1], vec3f({ 1 , 0, 0 }));
			//std::cout << w.m_mouse.position[0] << std::endl;
			b->setVariable("modelView", modelView);
			lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
		}
		else {
			lastMousePos = nullptr;
		}

		cmdBuf.wait(UINT32_MAX);
		cmdBuf.resetFence();

		ImGui::NewFrame();
		ImGui::Begin("Window");   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		
		ImGui::SliderFloat("x", &x, -10, 10); 
		ImGui::SliderFloat("y", &y, -10, 10);
		ImGui::SliderFloat("z", &z, -10, 10);

		ImGui::SliderFloat("red", &red, 0, 1);
		ImGui::SliderFloat("green", &green, 0, 1);
		ImGui::SliderFloat("blue", &blue, 0, 1);

		ImGui::SliderFloat("radius", &radius, 0, 1.5);
		ImGui::End();


		gui->prepareGui(d->getGraphicQueue(0), cmdBuf);

		

		cmdBuf.beginRecord();


		SSAOuni->setVariable("radius", radius);
		SSAOuni->update(cmdBuf);

		lightbuffer.setVariable("position", vec4f({ x,y,z,0.0 }));
		lightbuffer.setVariable("color", vec4f({ red,green,blue,0.0 }));
		lightbuffer.update(cmdBuf);

		if (updateUniform) {
			b->update(cmdBuf);
			updateUniform = false;
		}


		

		Gpass.draw(cmdBuf, *G, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } });


		
		SSAORenderPass->draw(cmdBuf, *SSAO, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } });

		
		Blurpass->draw(cmdBuf, *BlurBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } });

		Lightingpass->setSwapChainImage(*lightingBuffer, swapChainImage);
		Lightingpass->draw(cmdBuf, *lightingBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } });

		cmdBuf.endRecord();
		cmdBuf.submit(graphicQueue, wait_semaphore_infos, { cmdBuf.getSemaphore(0) });
		

		s->presentImage(presentQueue, swapChainImage, { cmdBuf.getSemaphore(0) });

	}

	d->end();
}
