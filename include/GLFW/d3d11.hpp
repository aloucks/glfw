#ifndef _glfw_d3d11_hpp_
#define _glfw_d3d11_hpp_

#include <d3d11.h>
#include "glfw3.h"

GLFWAPI ID3D11Device* d3dfwGetDevice(GLFWwindow* window);

GLFWAPI ID3D11DeviceContext* d3dfwGetImmediateContext(GLFWwindow* window);

GLFWAPI IDXGISwapChain* d3dfwGetSwapChain(GLFWwindow* window);

GLFWAPI ID3D11Texture2D* d3dfwGetStencilBuffer(GLFWwindow* window);

GLFWAPI ID3D11RenderTargetView* d3dfwGetRenderTargetView(GLFWwindow* window);

GLFWAPI ID3D11DepthStencilView* d3dfwGetDepthStencilView(GLFWwindow* window);

GLFWAPI void d3dfwResizeBuffers(GLFWwindow* window);

GLFWAPI void d3dfwGetViewports(GLFWwindow* window, UINT* pCount, D3D11_VIEWPORT** ppViewports);

GLFWAPI void d3dfwSetViewports(GLFWwindow* window, UINT count, D3D11_VIEWPORT* viewports);

#endif // _glfw_d3d11_hpp_