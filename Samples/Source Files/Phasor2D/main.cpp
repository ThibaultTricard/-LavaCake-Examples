#include "Framework/Framework.h"
#include "Phasor/Phasor2D.h"
#include "Helpers/Field.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;
using namespace LavaCake::Phasor;

#ifdef __APPLE__
std::string prefix ="../";
#else
std::string prefix ="";
#endif

vec2f slerp(vec2f A, vec2f B, float i) {
	float LA = sqrt(A[0]* A[0] + A[1]* A[1]);
	vec2f NA = A / LA;

	float LB = sqrt(B[0] * B[0] + B[1] * B[1]);
	vec2f NB = B / LB;

	float d = NA[0]*NB[0] + NA[1] * NB[1];

	d = min(max(d, -1.0), 1.0);
	
	float theta = acos(d) *(i);
	vec2f RV = NB - NA * d; 
	float LR = (RV[0] * RV[0] + RV[1] * RV[1]);
	if (LR != 0.0f) {
		RV = RV / (RV[0] * RV[0] + RV[1] * RV[1]);
	}
	return ((NA * cos(theta)) + (RV * sin(theta))) * (LA * (1.0f - i) + LB * i);

}

int main() {

	int nbFrames = 3;
	Framework::ErrorCheck::PrintError(true);
	Framework::Window w("LavaCake : Phasor", 512, 512);

	Framework::Device* d = Framework::Device::getDevice();
	d->initDevices(1, 1, w.m_windowParams);
	LavaCake::Framework::SwapChain* s = LavaCake::Framework::SwapChain::getSwapChain();
	s->init();

	Framework::Queue* queue = d->getGraphicQueue(0);
	VkQueue& present_queue = d->getPresentQueue()->getHandle();

	Queue* compute_queue = d->getComputeQueue(0);

	VkDevice logical = d->getLogicalDevice();
	VkExtent2D size = s->size();

	std::vector<Framework::CommandBuffer> commandBuffer = std::vector<Framework::CommandBuffer>(nbFrames);
	for (int i = 0; i < nbFrames; i++) {
		commandBuffer[i].addSemaphore();
		commandBuffer[i].addSemaphore();
	}

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

	Framework::VertexBuffer* quad_vertex_buffer = new Framework::VertexBuffer({ quad });
	quad_vertex_buffer->allocate(queue, commandBuffer[0]);

  
  Framework::UniformBuffer* sizeBuffer = new Framework::UniformBuffer();
  sizeBuffer->addVariable("width", 512);
  sizeBuffer->addVariable("height", 512);
  sizeBuffer->end();

  
  Helpers::ABBox<2> phasorBBox(vec2f({0.0f, 0.0f}), vec2f({10.0f, 10.0f}));
  
	float freq = 1.0f;

  std::vector<float> fvec ={ 6,1,1,6 };
  std::vector<vec2f> dvec ={
    vec2f({0.0f,1.0f}),vec2f({-1.0f,0.0f}),
    vec2f({1.0f,0.0f}),vec2f({0.0f,-1.0f})
  };
  

	

  Helpers::Field2DGrid<float>* F = new Helpers::Field2DGrid<float>(fvec, 2, 2, phasorBBox);
  Helpers::Field2DGrid<vec2f>* D = new Helpers::Field2DGrid<vec2f>(dvec, 2, 2, phasorBBox);

	F->sample({ 0.0f,0.0f });
	F->sample({ 0.0f,5.0f });
	F->sample({ 5.0f,0.0f });
	F->sample({ 5.0f,5.0f });


  Phasor2D ph(phasorBBox, F, freq, D);
  
  
	
  ph.init(queue, commandBuffer[0]);
  ph.phaseOptimisation(queue, commandBuffer[0], 100);
  ph.sample(queue, commandBuffer[0], phasorBBox, {512,512});

  Framework::Buffer* phasorBuffer = ph.getSampleBuffer();
  std::vector<float> phasorResult;
  phasorBuffer->readBack(queue, commandBuffer[0], phasorResult);
  
	//renderPass
	Framework::RenderPass* showPass = new Framework::RenderPass();

	Framework::GraphicPipeline* pipeline = new Framework::GraphicPipeline(vec3f({ 0,0,0 }) , vec3f({ float(size.width),float(size.height),1.0f }) , vec2f({ 0,0 }) , vec2f({ float(size.width),float(size.height) }));
	Framework::VertexShaderModule* vertexShader = new Framework::VertexShaderModule(prefix+"Data/Shaders/Phasor2D/shader.vert.spv");
	Framework::FragmentShaderModule* fragmentShader = new Framework::FragmentShaderModule(prefix+"Data/Shaders/Phasor2D/shader.frag.spv");
	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(quad_vertex_buffer->getBindingDescriptions(), quad_vertex_buffer->getAttributeDescriptions(), quad_vertex_buffer->primitiveTopology());
	pipeline->setVertices({ quad_vertex_buffer });
	pipeline->addTexelBuffer(phasorBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	pipeline->addUniformBuffer(sizeBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	showPass->addSubPass({ pipeline }, SA);

	showPass->compile();



	std::vector<Framework::FrameBuffer*> frameBuffers;
	for (int i = 0; i < nbFrames; i++) {
		frameBuffers.push_back(new Framework::FrameBuffer(s->size().width, s->size().height));
		showPass->prepareOutputFrameBuffer(*frameBuffers[i]);
	}


	commandBuffer[0].wait(2000000000);
	commandBuffer[0].resetFence();
	commandBuffer[0].beginRecord();

	sizeBuffer->update(commandBuffer[0]);

	commandBuffer[0].endRecord();
	commandBuffer[0].submit(compute_queue, {}, { commandBuffer[0].getSemaphore(1) });




	int f = 0;
	while (w.running()) {
		w.updateInput();
		f++;
		f = f % nbFrames;


		Framework::SwapChainImage& image = s->acquireImage();

		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});


		commandBuffer[f].wait(2000000000);
		commandBuffer[f].resetFence();
		commandBuffer[f].beginRecord();


		showPass->setSwapChainImage(*frameBuffers[f], image);


		showPass->draw(commandBuffer[f], *frameBuffers[f], vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

		commandBuffer[f].endRecord();

		commandBuffer[f].submit(queue, wait_semaphore_infos, { commandBuffer[f].getSemaphore(0) });
		

		PresentInfo present_info = {
			s->getHandle(),                                    // VkSwapchainKHR         Swapchain
			image.getIndex()                                   // uint32_t               ImageIndex
		};
		if (!PresentImage(present_queue, { commandBuffer[f].getSemaphore(0) }, { present_info })) {
			continue;
		}
	}

	d->end();


  return 0;
}
