#ifndef _d3d_context_h_
#define _d3d_context_h_

#define _GLFW_PLATFORM_FBCONFIG                 int                d3d
#define _GLFW_PLATFORM_CONTEXT_STATE            _GLFWcontextD3D d3d
#define _GLFW_PLATFORM_LIBRARY_CONTEXT_STATE    _GLFWlibraryD3D d3d

//========================================================================
// GLFW platform specific types
//========================================================================

//------------------------------------------------------------------------
// Platform-specific OpenGL context structure
//------------------------------------------------------------------------

typedef struct ID3D11Device              ID3D11Device;
typedef struct ID3D11DeviceContext       ID3D11DeviceContext;
typedef struct IDXGISwapChain            IDXGISwapChain;
typedef struct ID3D11Texture2D           ID3D11Texture2D;
typedef struct ID3D11RenderTargetView    ID3D11RenderTargetView;
typedef struct ID3D11DepthStencilView    ID3D11DepthStencilView;
typedef struct D3D11_VIEWPORT            D3D11_VIEWPORT;

typedef struct _GLFWcontextD3D
{
    ID3D11Device*           device;
    ID3D11DeviceContext*    context;
    IDXGISwapChain*         swapChain;
    ID3D11Texture2D*        depthStencilBuffer;
    ID3D11RenderTargetView* renderTargetView;
    ID3D11DepthStencilView* depthStencilView;
    UINT                    msaaSampleQuality;
    UINT                    msaaSampleCount;
    D3D11_VIEWPORT*         viewports;
    UINT                    viewportCount;

} _GLFWcontextD3D;

//------------------------------------------------------------------------
// Platform-specific library global data for DirectX
//------------------------------------------------------------------------
typedef struct _GLFWlibraryD3D
{
    //// opengl32.dll
    //struct {
    //    HINSTANCE   instance;
    //} opengl32;
    int dummy;

} _GLFWlibraryD3D;

void _d3dfwClear(_GLFWwindow* window);

//========================================================================
// Prototypes for platform specific internal functions
//========================================================================

#ifdef __cplusplus
extern "C" {
#endif

int _glfwInitContextAPI(void);
void _glfwTerminateContextAPI(void);
int _glfwCreateContext(_GLFWwindow* window,
                       const _GLFWctxconfig* ctxconfig,
                       const _GLFWfbconfig* fbconfig);
void _glfwDestroyContext(_GLFWwindow* window);
int _glfwAnalyzeContext(const _GLFWwindow* window,
                        const _GLFWctxconfig* ctxconfig,
                        const _GLFWfbconfig* fbconfig);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // _d3d_context.h