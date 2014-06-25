#define GL_TRUE  1
#define GL_FALSE 0
#define GLFW_INCLUDE_NONE

#include "../include/GLFW/d3d11.hpp"
#include <crtdbg.h>
#include <stdio.h>
#include <cstdint>

uint8_t dirty = 0;
D3D11_VIEWPORT viewport;

void errorCallback(int code, const char* msg) {
	printf("%d - %s\n", code, msg);
};

void frameBufferCallback(GLFWwindow* window, int width, int height) {
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb205075%28v=vs.85%29.aspx#Handling_Window_Resizing
	viewport.Width	  = static_cast<float>(width);
	viewport.Height	  = static_cast<float>(height);
	printf("frameBufferCallback()\n");
	dirty = 1;
}


int main() {

#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	void* oldFunction = glfwSetErrorCallback(errorCallback);
	int success = glfwInit();
	GLFWwindow* window = 0;
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	//glfwWindowHint(GLFW_DECORATED, GL_FALSE);
	//window = glfwCreateWindow(1920, 1200, "Test", NULL, NULL);
	window = glfwCreateWindow(800, 600, "glfw-d3d-window-create", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	else {
		glfwSetFramebufferSizeCallback(window, frameBufferCallback);
		ID3D11Device* device = d3dfwGetDevice(window);
		D3D_FEATURE_LEVEL fl = device->GetFeatureLevel();
		IDXGISwapChain* swapChain = d3dfwGetSwapChain(window);
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		viewport.Width	  = static_cast<float>(width);
		viewport.Height	  = static_cast<float>(height);
		viewport.MaxDepth = 1;
		viewport.MinDepth = 0;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		d3dfwSetViewports(window, 1, &viewport);
		while (!glfwWindowShouldClose(window)) {
			glfwWaitEvents();
			if (dirty) {
				printf("d3dfwResizeBuffers()\n");
				d3dfwResizeBuffers(window);
				dirty = 0;
			}
			glfwSwapBuffers(window);
		}
		glfwTerminate();
		return 0;
	}
};