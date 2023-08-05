#pragma once

#include <cstdlib>
#include <thread>
#include <vector>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "thorvg.h"

class TvgWindow {
protected:
	std::string glsl_version;
	GLFWwindow* window = nullptr;
	uint32_t* buffer = nullptr;
	GLuint texture = 0;
	int width = 0;
	int height = 0;
	std::unique_ptr<tvg::SwCanvas> canvas = nullptr;
	double lastTime = 0;
	double fps = 0;
	bool shouldClose = false;
	uint32_t clearColor = 0xff000000; // ARGB
public:
	static TvgWindow* instance;

	/**
	 * Create a new window
	 * @param width The width of the window
	 * @param height The height of the window
	 * @param name The name of the window
	 */
	TvgWindow(int width=800, int height=600, std::string name = "New Window");

	/**
	 * Destroy the window
	 */
	~TvgWindow();

	/**
	 * Call to close the window
	 * Should be called from within update as required
	 */
	void close();

	/**
	 * Run the window. Calls setup, then continuously calls update. Once the
	 * window is closed, cleanup is called.
	 */
	bool run();

	/**
	 * Override this to perform one-time setup actions
	 */
	virtual void setup() {}

	/**
	 * Override this to render thorVG on every loop.
	 * @param dt The number of seconds since the last call to update
	 */
	virtual void update(double dt) {}

	/**
	 * Override this to render the GUI on every loop.
	 * @param dt The number of seconds since the last call to update
	 */
	virtual void updateGui(double dt) {}

	/**
	 * Override this to perform one-time cleanup actions
	 */
	virtual void cleanup() {}

	/**
	 * Standard event handlers
	 * If you override these, make sure to call the inherited method
	 **/
	void onResize(int w, int h);
	virtual void onFilesDropped(std::vector<std::string> paths) {}
};
