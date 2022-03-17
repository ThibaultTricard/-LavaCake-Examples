#define LAVACAKE_WINDOW_MANAGER_GLFW
#include "Framework/Framework.h"
#include "AllHeaders.h"
#include "Common.h"
#include "Geometry/meshLoader.h"

#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="";
#endif

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;

int main() {
	int nbFrames = 3;
	ErrorCheck::printError(true);


	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(512, 512, "Lavacake: Post Process", nullptr, nullptr);

	GLFWSurfaceInitialisator surfaceInitialisator(window);

	Device* d = Device::getDevice();
	d->initDevices(0, 1, surfaceInitialisator);
	SwapChain* s = SwapChain::getSwapChain();
	s->init();
	VkExtent2D size = s->size();
	GraphicQueue queue = d->getGraphicQueue(0);
	PresentationQueue presentQueue = d->getPresentQueue();
	std::vector<CommandBuffer> commandBuffer = std::vector<CommandBuffer>(nbFrames);
	for (int i = 0; i < nbFrames; i++) {
		commandBuffer[i].addSemaphore();
	}

	std::pair<std::vector<float>, vertexFormat > sphere = Load3DModelFromObjFile(prefix+"Data/Models/sphere.obj", true, false, false, true);
	Geometry::Mesh_t* sphere_mesh = new Geometry::Mesh<Geometry::TRIANGLE>(sphere.first, sphere.second);

	

	VertexBuffer* sphere_vertex_buffer = new VertexBuffer(queue, commandBuffer[0],{ sphere_mesh });

	//Skybox Data

	std::pair<std::vector<float>, vertexFormat > sky = Load3DModelFromObjFile(prefix+"Data/Models/cube.obj", true, false, false, true);
	Geometry::Mesh_t* sky_mesh = new Geometry::Mesh<Geometry::TRIANGLE>(sky.first, sky.second);
	
	VertexBuffer* sky_vertex_buffer = new VertexBuffer(queue, commandBuffer[0], { sky_mesh });


	//PostProcessQuad
	Geometry::Mesh_t* quad = new Geometry::IndexedMesh<Geometry::TRIANGLE>(Geometry::P3UV);

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

	VertexBuffer* quad_vertex_buffer = new VertexBuffer(queue, commandBuffer[0],{ quad });
	

	//Uniform Buffer
	UniformBuffer uniforms = UniformBuffer();
	mat4 proj = PreparePerspectiveProjectionMatrix(static_cast<float>(size.width) / static_cast<float>(size.height),
		50.0f, 0.5f, 10.0f);
	mat4 modelView = PrepareTranslationMatrix(0.0f, 0.0f, -4.0f);
	uniforms.addVariable("modelView", modelView);
	uniforms.addVariable("projection",proj);
	uniforms.end();

	//SkyBox texture
	Image skyCubeMap = createCubeMap(queue, commandBuffer[0],prefix+"Data/Textures/Skansen/", 4);



	//Time PushConstant
	PushConstant* timeConstant = new PushConstant();
	float time = 0.0;
	timeConstant->addVariable("Time",  time );


	//camera PushConstant
	PushConstant* cameraConstant = new PushConstant();
	vec4f camera = vec4f({ 0.f,0.f,4.f,1.0 });
	cameraConstant->addVariable("camera", camera);

	//Color Attachment
	std::shared_ptr< Image > colorAttachemnt = std::make_shared < Image > (createAttachment(queue, commandBuffer[0], size.width, size.height, s->imageFormat(), attachmentType::COLOR_ATTACHMENT));
	

	//Render Pass
	RenderPass renderPass = RenderPass( );

	
	std::shared_ptr < GraphicPipeline > sphereRenderPipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule sphereVertex(prefix+"Data/Shaders/PostProcess/model.vert.spv");
	sphereRenderPipeline->setVertexModule(sphereVertex);

	FragmentShaderModule sphereFrag (prefix+"Data/Shaders/PostProcess/model.frag.spv");
	sphereRenderPipeline->setFragmentModule(sphereFrag);

	sphereRenderPipeline->setVerticesInfo(sphere_vertex_buffer->getBindingDescriptions(), sphere_vertex_buffer->getAttributeDescriptions(), sphere_vertex_buffer->primitiveTopology());

	sphereRenderPipeline->setPushContantInfo({ { cameraConstant->size() ,VK_SHADER_STAGE_FRAGMENT_BIT } });

	sphereRenderPipeline->setVertices({ { sphere_vertex_buffer , {{cameraConstant,VK_SHADER_STAGE_FRAGMENT_BIT}} } });
	sphereRenderPipeline->addUniformBuffer(uniforms, VK_SHADER_STAGE_VERTEX_BIT, 0);
	sphereRenderPipeline->addTextureBuffer(skyCubeMap, VK_SHADER_STAGE_FRAGMENT_BIT, 1);


	std::shared_ptr < GraphicPipeline > skyRenderPipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

	VertexShaderModule skyVertex(prefix+"Data/Shaders/PostProcess/skybox.vert.spv");
	skyRenderPipeline->setVertexModule(skyVertex);

	FragmentShaderModule skyFrag(prefix+"Data/Shaders/PostProcess/skybox.frag.spv");
	skyRenderPipeline->setFragmentModule(skyFrag);

	skyRenderPipeline->setVerticesInfo(sky_vertex_buffer->getBindingDescriptions(), sky_vertex_buffer->getAttributeDescriptions(), sky_vertex_buffer->primitiveTopology());

	skyRenderPipeline->setVertices({ sky_vertex_buffer });
	skyRenderPipeline->addUniformBuffer(uniforms, VK_SHADER_STAGE_VERTEX_BIT, 0);
	skyRenderPipeline->addTextureBuffer(skyCubeMap, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	skyRenderPipeline->setCullMode(VK_CULL_MODE_FRONT_BIT);

	SubpassAttachment SA;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;

	renderPass.addSubPass({ sphereRenderPipeline,skyRenderPipeline }, SA);

	std::shared_ptr < GraphicPipeline > postProcessPipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
	VertexShaderModule postProcessVertex(prefix+"Data/Shaders/PostProcess/postprocess.vert.spv");
	postProcessPipeline->setVertexModule(postProcessVertex);

	FragmentShaderModule postProcessFrag(prefix+"Data/Shaders/PostProcess/postprocess.frag.spv");
	postProcessPipeline->setFragmentModule(postProcessFrag);

	postProcessPipeline->setPushContantInfo({ { timeConstant->size() ,VK_SHADER_STAGE_FRAGMENT_BIT } });
	
	postProcessPipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());

	postProcessPipeline->setVertices({ { quad_vertex_buffer, {{timeConstant,VK_SHADER_STAGE_FRAGMENT_BIT}} } });
	
	postProcessPipeline->addAttachment(colorAttachemnt, VK_SHADER_STAGE_FRAGMENT_BIT, 0);


	SA = SubpassAttachment();
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.showOnScreenIndex = 0;
	SA.addInput = true;

	renderPass.addSubPass({ postProcessPipeline }, SA, { 0 });

	renderPass.addDependencies(
		0,
		1,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT 
	);
	renderPass.addDependencies(
		VK_SUBPASS_EXTERNAL,  
		1, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_ACCESS_MEMORY_READ_BIT, 
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_DEPENDENCY_BY_REGION_BIT 
	);
	renderPass.addDependencies(
		1,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_ACCESS_MEMORY_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT 
	);
	renderPass.compile();

	std::vector<FrameBuffer*> frameBuffers;
	for (int i = 0; i < nbFrames; i++) {
		frameBuffers.push_back(new FrameBuffer(s->size().width, s->size().height));
		renderPass.prepareOutputFrameBuffer(queue, commandBuffer[0], *frameBuffers[i]);
	}

	bool updateUniformBuffer = true;
	int f = 0;
	vec2d* lastMousePos = nullptr;

	vec2d polars = vec2d({ 0.0,0.0 });
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		f++;
		f = f % nbFrames;

		VkDevice logical = d->getLogicalDevice();
		VkExtent2D size = s->size();

		

		time +=0.001f;
		timeConstant->setVariable("Time",  time);

		/*if (mouse->leftButton) {
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
			uniforms->setVariable("modelView", modelView);
			lastMousePos = new vec2d({ mouse->position[0], mouse->position[1] });
		}
		else {
			lastMousePos = nullptr;
		}*/



		commandBuffer[f].wait(2000000000);
		commandBuffer[f].resetFence();
		commandBuffer[f].beginRecord();

		const SwapChainImage& image = s->acquireImage();
		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),																	// VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});
		
		if (updateUniformBuffer) {
			uniforms.update(commandBuffer[f]);
			updateUniformBuffer = false;
		}

		
		renderPass.setSwapChainImage(*frameBuffers[f], image);
		renderPass.draw(commandBuffer[f], *frameBuffers[f], vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } , { 0.1f, 0.2f, 0.3f, 1.0f } });



		commandBuffer[f].endRecord();


		commandBuffer[f].submit(queue, wait_semaphore_infos, { commandBuffer[f].getSemaphore(0) });
		

		s->presentImage(presentQueue, image, { commandBuffer[f].getSemaphore(0) });
	}

	d->waitForAllCommands();
}

