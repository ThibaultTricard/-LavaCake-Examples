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

	GLFWwindow* window = glfwCreateWindow(512, 512, "LavaCake imgui", nullptr, nullptr);

	GLFWSurfaceInitialisator surfaceInitialisator(window);

	ErrorCheck::printError(true, 8);

	int  debug = 0;

	int nbFrames = 1;
	LavaCake::Framework::Device* d = LavaCake::Framework::Device::getDevice();
	d->initDevices(0, 1, surfaceInitialisator);
	LavaCake::Framework::SwapChain* s = LavaCake::Framework::SwapChain::getSwapChain();
	s->init(); 
	VkExtent2D size = s->size();
	GraphicQueue queue = d->getGraphicQueue(0);
	PresentationQueue presentQueue = d->getPresentQueue();
	std::vector<CommandBuffer> commandBuffer = std::vector<CommandBuffer>(nbFrames);
	for (int i = 0; i < nbFrames; i++) {
		commandBuffer[i].addSemaphore();
	}
	ImGuiWrapper* gui = new ImGuiWrapper(d->getGraphicQueue(0), commandBuffer[0], vec2i({ 512 ,512 }), vec2i({ (int)size.width ,(int)size.height }));

	prepareInputs(window);

	Mesh_t* triangle = new Mesh<TRIANGLE>(LavaCake::Geometry::PC3);
	triangle->appendVertex({ -0.75f, 0.75f , 0.0f, 1.0f	, 0.0f	, 0.0f });
	triangle->appendVertex({ 0.75f,	0.75f , 0.0f,  0.0f	, 1.0f	, 0.0f });
	triangle->appendVertex({ 0.0f , -0.75f, 0.0f, 0.0f	, 0.0f	, 1.0f });

	VertexBuffer* triangle_vertex_buffer = new VertexBuffer(queue, commandBuffer[0],{ triangle });

	RenderPass* pass = new RenderPass();
	std::shared_ptr < GraphicPipeline > pipeline = std::make_shared < GraphicPipeline >(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));

  VertexShaderModule vertexShader = VertexShaderModule(prefix+"Data/Shaders/helloworld/shader.vert.spv");
	FragmentShaderModule fragmentShader = FragmentShaderModule(prefix+"Data/Shaders/helloworld/shader.frag.spv");


	pipeline->setVertexModule(vertexShader);
	pipeline->setFragmentModule(fragmentShader);
	pipeline->setVerticesInfo(triangle_vertex_buffer->getBindingDescriptions(), triangle_vertex_buffer->getAttributeDescriptions(), triangle_vertex_buffer->primitiveTopology());

	pipeline->setVertices({ triangle_vertex_buffer });



	SubpassAttachment SA;
	SA.showOnScreen = true;
	SA.nbColor = 1;
	SA.storeColor = true;
	SA.useDepth = true;
	SA.showOnScreenIndex = 0;

	pass->addSubPass({ pipeline, gui->getPipeline()}, SA);
	pass->addDependencies(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
	pass->addDependencies(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

	pass->compile();

	std::vector<FrameBuffer*> frameBuffers;
	for (int i = 0; i < nbFrames; i++) {
		frameBuffers.push_back(new FrameBuffer(s->size().width, s->size().height));
		pass->prepareOutputFrameBuffer(queue, commandBuffer[0], *frameBuffers[i]);
	}

	int f = 0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		f++;
		f = f % nbFrames;
		commandBuffer[f].wait(2000000000);
		commandBuffer[f].resetFence();


		bool show_demo_window = true;
		bool show_another_window = true;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		
		{
			ImGuiIO& io = ImGui::GetIO();
			IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

			// Setup display size (every frame to accommodate for window resizing)
			int width, height;
			int display_w, display_h;
			glfwGetWindowSize(window, &width, &height);
			glfwGetFramebufferSize(window, &display_w, &display_h);
			io.DisplaySize = ImVec2((float)width, (float)height);
			if (width > 0 && height > 0)
				io.DisplayFramebufferScale = ImVec2((float)display_w / width, (float)display_h / height);
		}
		ImGui::NewFrame();

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		
		gui->prepareGui(d->getGraphicQueue(0), commandBuffer[f]);
		
		
		VkDevice logical = d->getLogicalDevice();
		VkSwapchainKHR& swapchain = s->getHandle();
		const SwapChainImage& image = s->acquireImage();


	



		std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
		wait_semaphore_infos.push_back({
			image.getSemaphore(),                     // VkSemaphore            Semaphore
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT					// VkPipelineStageFlags   WaitingStage
			});

		
		commandBuffer[f].beginRecord();

		
		

		pass->setSwapChainImage(*frameBuffers[f], image);

		pass->draw(commandBuffer[f], *frameBuffers[f], vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });


		commandBuffer[f].endRecord();

		commandBuffer[f].submit(queue, wait_semaphore_infos, { commandBuffer[f].getSemaphore(0) });
		

		s->presentImage(presentQueue, image, { commandBuffer[f].getSemaphore(0) });

		debug++;
	}
	d->end();
	
}
