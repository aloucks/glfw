#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef _GLFW_USE_D3D11
 #include "../include/GLFW/d3d11.hpp" // <d3d11.h> is included here
 #pragma comment(lib, "d3d11")
#else
 #error "_GLFW_USE_D3D11 is not defined"
#endif

#include "wchar.h"
#include <stdio.h>
#include <winerror.h>

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }
#define ReturnOnFailure(hr) \
    if (FAILED(hr))         \
    {                       \
        error(hr);          \
        return GL_FALSE;    \
    }


// Return false on failure
#define HRF(hr)             \
    if (FAILED(hr))         \
    {                       \
        error(hr);          \
        return GL_FALSE;    \
    }                       \

// Return (void) on failure;
#define HRV(hr)             \
    if (FAILED(hr))         \
    {                       \
        error(hr);          \
        return;             \
    }                       \

static void error(HRESULT hr) {
    // TODO : This doesn't work (well)
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
    {
        hr = HRESULT_CODE(hr);
    }
    
    WCHAR buffer[2048];
    if(FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL, 
        hr, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&buffer,
        sizeof(buffer) * sizeof(WCHAR),
        NULL) != 0)
    {
        char* msg = _glfwCreateUTF8FromWideString(buffer);
        _glfwInputError(GLFW_PLATFORM_ERROR, "Win32 DirectX: %s", msg);
        free(msg);
        //LocalFree(buffer);
    }
    else
    {
        _glfwInputError(GLFW_PLATFORM_ERROR, "Win32 DirectX: Unknown Error");
    }
    
}

static void setSampleCountAndQuality(const _GLFWwindow* window, DXGI_SAMPLE_DESC* sd) {

    int count = 1, quality = 0;

    if (window->d3d.msaaSampleCount > 0) {
        count = window->d3d.msaaSampleCount;
        if (window->d3d.msaaSampleQuality > 0) {
            quality = window->d3d.msaaSampleQuality - 1;
        }
    }

    sd->Count = count;
    sd->Quality = quality;
}

static void setContextVersion(_GLFWwindow* window, D3D_FEATURE_LEVEL d3dFeatureLevel) {
    window->context.api         = GLFW_OPENGL_API;
    window->context.revision    = 0;
    window->context.robustness  = 0;

    // D3D_FEATURE_LEVEL_11_1 is not present in the VS 2010 June DirectX SDK
    // Assume if the level is greater than 11_0 that at least 11_1 is available
    if (d3dFeatureLevel > D3D_FEATURE_LEVEL_11_0) {
        window->context.major = 11; 
        window->context.minor = 1;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_11_0) {
        window->context.major = 11; 
        window->context.minor = 0;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_10_1) {
        window->context.major = 10; 
        window->context.minor = 1;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_10_0) {
        window->context.major = 10; 
        window->context.minor = 0;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_9_3) {
        window->context.major = 9; 
        window->context.minor = 3;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_9_2) {
        window->context.major = 9; 
        window->context.minor = 2;
    }
    else if (d3dFeatureLevel >= D3D_FEATURE_LEVEL_9_1) {
        window->context.major = 9; 
        window->context.minor = 1;
    }
    else {
        window->context.major = 0; 
        window->context.minor = 0;
    }
}

#ifdef __cplusplus
extern "C" {
#endif

void _d3dfwClear(_GLFWwindow* window) {
    const FLOAT black[] = {0, 0, 0, 0};
    window->d3d.context->ClearRenderTargetView(window->d3d.renderTargetView, reinterpret_cast<const float*>(&black));
}

int _glfwInitContextAPI(void)
{
    return GL_TRUE;
}

void _glfwTerminateContextAPI(void)
{
}

int _glfwCreateContext(_GLFWwindow* window,
                       const _GLFWctxconfig* ctxconfig,
                       const _GLFWfbconfig* fbconfig)
{
    UINT createDeviceFlags = 0;
    IDXGIDevice* dxgiDevice = 0;
    IDXGIAdapter* dxgiAdapter = 0;
    IDXGIFactory* dxgiFactory = 0;

    D3D_FEATURE_LEVEL d3dFeatureLevel;
    DXGI_SWAP_CHAIN_DESC sd;
    D3D_DRIVER_TYPE d3dDriverType;

    int width, height;
    
    if (ctxconfig->debug)
    {
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    if (TRUE /* TODO: check driverType hints*/)
    {
        d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    }
    
    HRF(D3D11CreateDevice(
        0,                    // [in] Use the default adaptor
        d3dDriverType,
        0,                    // [in] HMODULE software rasterizer
        createDeviceFlags,
        0,                    // [in] D3D_FEATURE_LEVEL pFeatureLevels
        0,                    // [in] Number of pFeatureLevels
        D3D11_SDK_VERSION,
        &window->d3d.device,
        &d3dFeatureLevel,
        &window->d3d.context));

    setContextVersion(window, d3dFeatureLevel);
        
    window->d3d.msaaSampleCount = fbconfig->samples;

    if (window->d3d.msaaSampleCount > 0)
    {
        HRF(window->d3d.device->CheckMultisampleQualityLevels(
            DXGI_FORMAT_R8G8B8A8_UNORM, 
            window->d3d.msaaSampleCount, 
            &window->d3d.msaaSampleQuality));
    }

    setSampleCountAndQuality(window, &sd.SampleDesc);

    _glfwPlatformGetWindowSize(window, &width, &height);

    sd.BufferDesc.Width                   = width;
    sd.BufferDesc.Height                  = height;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount                        = 1;
    sd.OutputWindow                       = window->win32.handle;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags                              = 0;

    HRF(window->d3d.device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
    HRF(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));
    HRF(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));
    HRF(dxgiFactory->CreateSwapChain(window->d3d.device, &sd, &window->d3d.swapChain));

    ReleaseCOM(dxgiDevice);
    ReleaseCOM(dxgiAdapter);
    ReleaseCOM(dxgiFactory);

    d3dfwResizeBuffers((GLFWwindow*)window);

    return GL_TRUE;
}

void _glfwDestroyContext(_GLFWwindow* window)
{
    ReleaseCOM(window->d3d.renderTargetView);
    ReleaseCOM(window->d3d.depthStencilView);
    ReleaseCOM(window->d3d.depthStencilBuffer);
    ReleaseCOM(window->d3d.swapChain);
    ReleaseCOM(window->d3d.context);
    ReleaseCOM(window->d3d.device);
}

int _glfwAnalyzeContext(const _GLFWwindow* window,
                        const _GLFWctxconfig* ctxconfig,
                        const _GLFWfbconfig* fbconfig)
{
    return _GLFW_RECREATION_NOT_NEEDED;
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////


void _glfwPlatformMakeContextCurrent(_GLFWwindow* window)
{
    //if (window)
    //    wglMakeCurrent(window->wgl.dc, window->wgl.context);
    //else
    //    wglMakeCurrent(NULL, NULL);

    _glfwSetCurrentContext(window);
}

void _glfwPlatformSwapBuffers(_GLFWwindow* window)
{
    window->d3d.swapChain->Present(0, 0);
}

void _glfwPlatformSwapInterval(int interval)
{
    _GLFWwindow* window = _glfwPlatformGetCurrentContext();

#if !defined(_GLFW_USE_DWM_SWAP_INTERVAL)
    if (_glfwIsCompositionEnabled() && interval)
    {
        // Don't enabled vsync when desktop compositing is enabled, as it leads
        // to frame jitter
        return;
    }
#endif

    //if (window->wgl.EXT_swap_control)
    //    window->wgl.SwapIntervalEXT(interval);
}

int _glfwPlatformExtensionSupported(const char* extension)
{
    return GL_FALSE;
}

GLFWglproc _glfwPlatformGetProcAddress(const char* procname)
{
    return 0;
}


#ifdef __cplusplus
}; // extern "C"
#endif

//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI ID3D11Device* d3dfwGetDevice(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.device;
}

GLFWAPI ID3D11DeviceContext* d3dfwGetImmediateContext(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.context;
}

GLFWAPI IDXGISwapChain* d3dfwGetSwapChain(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.swapChain;
}

GLFWAPI ID3D11Texture2D* d3dfwGetStencilBuffer(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.depthStencilBuffer;
}

GLFWAPI ID3D11RenderTargetView* d3dfwGetRenderTargetView(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.renderTargetView;
}

GLFWAPI ID3D11DepthStencilView* d3dfwGetDepthStencilView(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    return w->d3d.depthStencilView;
}

GLFWAPI void d3dfwResizeBuffers(GLFWwindow* window) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    DXGI_SWAP_CHAIN_DESC sd;
    D3D11_TEXTURE2D_DESC dd; // depth stencil desc
    ID3D11Texture2D* backBuffer;

    w->d3d.swapChain->GetDesc(&sd);

    ReleaseCOM(w->d3d.renderTargetView);
    ReleaseCOM(w->d3d.depthStencilView);
    ReleaseCOM(w->d3d.depthStencilBuffer);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (width  < 1) width  = 1;
    if (height < 1) height = 1;
    sd.BufferDesc.Width  = static_cast<UINT>(width);
    sd.BufferDesc.Height = static_cast<UINT>(height);

    HRV(w->d3d.swapChain->ResizeBuffers(1, width, height, sd.BufferDesc.Format, 0));
    HRV(w->d3d.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
    HRV(w->d3d.device->CreateRenderTargetView(backBuffer, 0, &w->d3d.renderTargetView));

    ReleaseCOM(backBuffer);

    dd.Width        = sd.BufferDesc.Width;
    dd.Height       = sd.BufferDesc.Height;
    dd.MipLevels    = 1;
    dd.ArraySize    = 1;
    dd.Format       = DXGI_FORMAT_D24_UNORM_S8_UINT;

    setSampleCountAndQuality(w, &dd.SampleDesc);

    dd.Usage          = D3D11_USAGE_DEFAULT;
    dd.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
    dd.CPUAccessFlags = 0; 
    dd.MiscFlags      = 0;

    HRV(w->d3d.device->CreateTexture2D(&dd, 0, &w->d3d.depthStencilBuffer));
    HRV(w->d3d.device->CreateDepthStencilView(w->d3d.depthStencilBuffer, 0, &w->d3d.depthStencilView));

    w->d3d.context->OMSetRenderTargets(1, &w->d3d.renderTargetView, w->d3d.depthStencilView);

    if (w->d3d.viewportCount > 0 && w->d3d.viewports != NULL) {
        w->d3d.context->RSSetViewports(w->d3d.viewportCount, w->d3d.viewports);
    }

}

GLFWAPI void d3dfwGetViewports(GLFWwindow* window, UINT* pCount, D3D11_VIEWPORT** ppViewports) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    *pCount = w->d3d.viewportCount;
    *ppViewports = w->d3d.viewports;
}

GLFWAPI void d3dfwSetViewports(GLFWwindow* window, UINT count, D3D11_VIEWPORT* viewports) {
    _GLFWwindow* w = (_GLFWwindow*) window;
    w->d3d.viewportCount = count;
    w->d3d.viewports = viewports;
    w->d3d.context->RSSetViewports(w->d3d.viewportCount, w->d3d.viewports);
}