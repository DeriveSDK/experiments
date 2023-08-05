#include "TvgWindow.h"

TvgWindow* TvgWindow::instance = nullptr;

void glfwOnFramebufferResize(GLFWwindow* window, int w, int h) {
	TvgWindow::instance->onResize(w, h);
}

void glfwOnFilesDropped(GLFWwindow* window, int count, const char** paths) {
	std::vector<std::string> filenames;
	for (int i = 0; i < count; i++) filenames.push_back(std::string(paths[i]));
	TvgWindow::instance->onFilesDropped(filenames);
}

TvgWindow::TvgWindow(int w, int h, std::string name) {
	if (!glfwInit()) return;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_DEPTH_BITS, 0);

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	window = glfwCreateWindow(w, h, name.c_str(), NULL, NULL);
	if (!window) {
		glfwTerminate();
		return;
	}

	glfwSetFramebufferSizeCallback(window, glfwOnFramebufferResize);
	glfwSetDropCallback(window, glfwOnFilesDropped);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glEnable(GL_TEXTURE_2D);

	tvg::Initializer::init(tvg::CanvasEngine::Sw, std::thread::hardware_concurrency());
	canvas = tvg::SwCanvas::gen();

	glfwGetWindowSize(window, &width, &height);
	onResize(width, height);

	lastTime = glfwGetTime();

	TvgWindow::instance = this;
}

TvgWindow::~TvgWindow() {
	glfwDestroyWindow(window);
	glfwTerminate();

	tvg::Initializer::term(tvg::CanvasEngine::Sw);

	delete[] buffer;
}

void TvgWindow::close() {
	shouldClose = true;
}

void TvgWindow::onResize(int w, int h) {
	// Create a new framebuffer
	width = w;
	height = h;
	delete[] buffer;
	buffer = new uint32_t[width * height];

	// Create a new texture
	glDeleteTextures(1, &texture);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		width, height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		(void*)buffer
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// reattach buffer to tvg canvas
	canvas->target(buffer, width, width, height, tvg::SwCanvas::ABGR8888);

	// Set projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, width, height);
}

bool TvgWindow::run() {
	// Exit if there was an error during setup
	if (!TvgWindow::instance) return false;

	// Set up Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version.c_str());

	// User setup
	setup();

	// Main render loop
	while (!glfwWindowShouldClose(window))
	{
		// Clear
		glClearColor(((clearColor >> 16) & 0xff) / 255.0f, ((clearColor >> 8) & 0xff) / 255.0f, (clearColor & 0xff) / 255.0f, ((clearColor >> 24) & 0xff) / 255.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// User render loop
		double thisTime = glfwGetTime();
		fps = 1.0 / (thisTime - lastTime);
		update(thisTime - lastTime);

		// Render the buffer to a texture and display it
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0, 0,
			width, height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			(void*)buffer
		);
		glBegin(GL_QUADS); 
		glTexCoord2f(0, 0); glVertex2f(0, 0);
		glTexCoord2f(1, 0); glVertex2f(width, 0);
		glTexCoord2f(1, 1); glVertex2f(width, height);
		glTexCoord2f(0, 1); glVertex2f(0, height);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// User render GUI loop
		updateGui(thisTime - lastTime);

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap framebuffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		lastTime = thisTime;

		// User closed window?
		if (shouldClose) glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	// User cleanup
	cleanup();

	// Clean up Dear ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return true;
}
