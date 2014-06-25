#define GL_TRUE  1
#define GL_FALSE 0
#define GLFW_INCLUDE_NONE

#include "../include/GLFW/d3d11.hpp"
#include <crtdbg.h>
#include <stdio.h>
#include <cstdint>

extern "C" {
#include "../deps/tinycthread.h"
}

#if _MSC_VER < 1700
 #include <xnamath.h>
 #include <D3Dcompiler.h>
 #include <D3DX11.h>
 #define XNA_MATH
#else
 #include <DirectXMath.h>
 #include <d3dcompiler.h>
 using namespace DirectX;
#endif

#include "d3d-cube.ps.h"
#include "d3d-cube.vs.h"

#pragma comment(lib,"d3dcompiler.lib")

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

// Return false on failure
#define HRF(hr)           \
    if (FAILED(hr))       \
    {                     \
        error(hr);        \
        return GL_FALSE;  \
    }                     \

// Return (void) on failure;
#define HRV(hr)           \
    if (FAILED(hr))       \
    {                     \
        error(hr);        \
        return;           \
    }                     \

uint8_t dirty = 0;
D3D11_VIEWPORT viewport;
ID3D11Buffer* mBoxVB;
ID3D11Buffer* mBoxIB;
ID3D11Buffer* mWorldViewProjCB;
ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;
ID3D11InputLayout* mInputLayout;
XMFLOAT4X4 mWorld;
XMFLOAT4X4 mView;
XMFLOAT4X4 mProj;
float mTheta    = 1.50f * XM_PI;
float mPhi      = 0.25f * XM_PI;
float mRadius   = 5.00f;

void error(HRESULT hr) {
    // TODO : This doesn't work (well)
    if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
    {
        hr = HRESULT_CODE(hr);
    }
    
    TCHAR buffer[2048];
    if(FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL, 
        hr, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer,
        sizeof(buffer) * sizeof(TCHAR),
        NULL) != 0)
    {
        printf_s("%s\n", buffer);
    }
    else
    {
        printf_s("Unknown error\n");
    }
}

namespace Colors
{
    XMGLOBALCONST XMVECTORF32 White     = {1.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Black     = {0.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Red       = {1.0f, 0.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Green     = {0.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Blue      = {0.0f, 0.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Yellow    = {1.0f, 1.0f, 0.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Cyan      = {0.0f, 1.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Magenta   = {1.0f, 0.0f, 1.0f, 1.0f};
    XMGLOBALCONST XMVECTORF32 Silver    = {0.75f, 0.75f, 0.75f, 1.0f};
    XMGLOBALCONST XMVECTORF32 LightSteelBlue = {0.69f, 0.77f, 0.87f, 1.0f};
};

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

int buildGeometryBuffers(ID3D11Device* device) {

    // Create vertex buffer
#ifdef XNA_MATH
    Vertex vertices[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White   },
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black   },
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red     },
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green   },
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue    },
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow  },
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan    },
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta }
    };
#else
    Vertex vertices[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   },
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue )   },
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan  )  },
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
    };
#endif

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * 8;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    vbd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = vertices;
    HRF(device->CreateBuffer(&vbd, &vinitData, &mBoxVB));

    // Create the index buffer

    UINT indices[] = {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3, 
        4, 3, 7
    };

    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * 36;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    ibd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {0};
    iinitData.pSysMem = indices;

    HRF(device->CreateBuffer(&ibd, &iinitData, &mBoxIB));

    /**/
    D3D11_BUFFER_DESC cbd = {0};
    cbd.ByteWidth            = sizeof(XMMATRIX); // worldViewProj
    cbd.Usage                = D3D11_USAGE_DEFAULT;
    cbd.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags        = 0;
    cbd.MiscFlags            = 0;
    cbd.StructureByteStride = 0;
    /**/

    /*
    D3D11_BUFFER_DESC cbd = {0};
    cbd.ByteWidth            = sizeof(XMMATRIX); // worldViewProj
    cbd.Usage                = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags        = D3D11_CPU_ACCESS_WRITE;
    cbd.MiscFlags            = 0;
    cbd.StructureByteStride = 0;
    */

    HRF(device->CreateBuffer(&cbd, NULL, &mWorldViewProjCB));

    return 1;
}

int compileShaders(ID3D11Device* device) {

    DWORD shaderFlags = D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
    ID3DBlob* compiledShader = 0;
    ID3DBlob* compilationMsgs = 0;

    const D3D_SHADER_MACRO defines[] = 
    {
        "EXAMPLE_DEFINE", "1"
    };

    // Create the vertex input layout.
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };


    /*
    HRESULT hr;

    hr = D3DCompileFromFile(
        L"d3d-cube.vs.hlsl", 
        0,                                   // no defines 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,    
        "VS",                                // entry function
        "vs_5_0",                            // profile
        shaderFlags, 
        0,                                   // flags2 not used 
        &compiledShader, 
        &compilationMsgs);

    HRF(device->CreateVertexShader(
        compiledShader->GetBufferPointer(),
        compiledShader->GetBufferSize(),
        NULL,
        &vertexShader));
    if (compilationMsgs != 0) {
        printf("vertex shader failed to compile: %s\n", compilationMsgs->GetBufferPointer());
    }
    HRF(hr);

    HRF(device->CreateInputLayout(
        vertexDesc,
        ARRAYSIZE(vertexDesc),
        compiledShader->GetBufferPointer(),
        compiledShader->GetBufferSize(),
        &mInputLayout));

    hr = D3DCompileFromFile(
        L"d3d-cube.ps.hlsl", 
        0,                                   // no defines 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,    
        "PS",                                // entry function
        "ps_5_0",                            // profile
        shaderFlags, 
        0,                                   // flags2 not used 
        &compiledShader, 
        &compilationMsgs);
    if (compilationMsgs != 0) {
        printf("pixel shader failed to compile: %s\n", compilationMsgs->GetBufferPointer()); 
    }
    HRF(hr);

    HRF(device->CreatePixelShader(
        compiledShader->GetBufferPointer(),
        compiledShader->GetBufferSize(),
        NULL,
        &pixelShader));
    */

    HRF(device->CreateVertexShader(
        &g_VS,
        sizeof(g_VS),
        NULL,
        &vertexShader));

    HRF(device->CreateInputLayout(
        vertexDesc,
        ARRAYSIZE(vertexDesc),
        &g_VS,
        sizeof(g_VS),
        &mInputLayout));
    
    HRF(device->CreatePixelShader(
        &g_PS,
        sizeof(g_PS),
        NULL,
        &pixelShader));

    return 1;
}

void updateProj() {
    float aspectRatio = viewport.Width / viewport.Height;
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*XM_PI, aspectRatio, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void errorCallback(int code, const char* msg) {
    printf("%d - %s\n", code, msg);
};

void frameBufferCallback(GLFWwindow* window, int width, int height) {
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb205075%28v=vs.85%29.aspx#Handling_Window_Resizing
    viewport.Width      = static_cast<float>(width);
    viewport.Height     = static_cast<float>(height);
    printf("frameBufferCallback()\n");
    dirty = 1;

    updateProj();
}


void updateScene() {

    static double base = glfwGetTime();

    double now = glfwGetTime();
    float dx = static_cast<float>(now) -  static_cast<float>(base);
    base = now;

    mTheta += dx;

    float x = mRadius*sinf(mPhi)*cosf(mTheta);
    float z = mRadius*sinf(mPhi)*sinf(mTheta);
    float y = mRadius*cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos    = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);
}

void drawScene(GLFWwindow* window) {
    // http://code.msdn.microsoft.com/windowsapps/Direct3D-Tutorial-Sample-08667fb0/sourcecode?fileId=44730&pathId=904833696

    ID3D11DeviceContext* md3dImmediateContext = d3dfwGetImmediateContext(window);
    ID3D11RenderTargetView* mRenderTargetView = d3dfwGetRenderTargetView(window);
    ID3D11DepthStencilView* mDepthStencilView = d3dfwGetDepthStencilView(window);

    // Set constants
    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX view  = XMLoadFloat4x4(&mView);
    XMMATRIX proj  = XMLoadFloat4x4(&mProj);

    // XMMATRIX worldViewProj = XMMatrixTranspose(world*view*proj);
    // The documentation recommends not using the overloads operators.
    XMMATRIX worldView = XMMatrixMultiply(world, view);
    XMMATRIX worldViewProj = XMMatrixTranspose(XMMatrixMultiply(worldView, proj));

    /**/
    // Update the worldVewProj constant buffer
    md3dImmediateContext->UpdateSubresource(
        mWorldViewProjCB, 
        0,
        0,
        &worldViewProj,
        0,
        0);

    /**/
    

    // This shouldn't be needed on every call since we're only rendering to 
    // a single target. The default render target is set when the window is created
    // or when d3dfwResizeBuffers() is called.
    md3dImmediateContext->OMSetRenderTargets(
        1,
        &mRenderTargetView,
        mDepthStencilView);

    md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
    md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    // Index Array
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
    md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);
    md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Vertex Shader
    md3dImmediateContext->VSSetConstantBuffers(0, 1, &mWorldViewProjCB);
    md3dImmediateContext->VSSetShader(vertexShader, NULL, 0);

    // Pixel Shader
    md3dImmediateContext->PSSetShader(pixelShader, NULL, 0);

    // Draw
    md3dImmediateContext->DrawIndexed(
        36, // number of cube verticies
        0,
        0);
}

int render(void* glfwWindow) {
    GLFWwindow* window = (GLFWwindow*) glfwWindow;
    while (!glfwWindowShouldClose(window)) {
        if (dirty) {
            printf("d3dfwResizeBuffers()\n");
            d3dfwResizeBuffers(window);
            dirty = 0;
        }

        updateScene();
        drawScene(window);
        glfwSwapBuffers(window);
    }
    return 0;
}

void cleanup() {
    ReleaseCOM(mBoxVB);
    ReleaseCOM(mBoxIB);
    ReleaseCOM(pixelShader);
    ReleaseCOM(vertexShader);
    ReleaseCOM(mInputLayout);
    ReleaseCOM(mWorldViewProjCB);
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
    window = glfwCreateWindow(800, 600, "glfw-d3d-cube", NULL, NULL);
    if (!window) {
        cleanup();
        glfwTerminate();
        return -1;
    }
    else {
        XMMATRIX I = XMMatrixIdentity();
        XMStoreFloat4x4(&mWorld, I);
        XMStoreFloat4x4(&mView, I);
        XMStoreFloat4x4(&mProj, I);

        ID3D11Device* device = d3dfwGetDevice(window);
        int geometrySuccess = buildGeometryBuffers(device);
        int shaderSuccess = geometrySuccess && compileShaders(device);
        if (shaderSuccess) {
            glfwSetFramebufferSizeCallback(window, frameBufferCallback);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            viewport.Width    = static_cast<float>(width);
            viewport.Height   = static_cast<float>(height);
            viewport.MaxDepth = 1;
            viewport.MinDepth = 0;
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            d3dfwSetViewports(window, 1, &viewport);
            updateProj();
            //std::thread renderThread(render, window);
            thrd_t renderThread;
            thrd_create(&renderThread, render, window);
            while (!glfwWindowShouldClose(window)) {
                glfwWaitEvents();
                int close = glfwGetKey(window, GLFW_KEY_ESCAPE);
                if (close == GLFW_PRESS) {
                    glfwSetWindowShouldClose(window, GL_TRUE);
                }
            }
            //renderThread.join();
            thrd_join(renderThread, NULL);
        }
        cleanup();
        glfwTerminate();
        return 0;
    }
};