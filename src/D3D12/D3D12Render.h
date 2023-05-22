#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "D3D12/tools/ToolForD12.h"
#include"D3D12/tools/GameTimer.h"
#include"MeshGeometry.h"
#include"D3D12/tools/MyMath.h"
#include"Camera.h"

#include"ObjectConsts.h"
#include"UploadBuffer.h"


// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


using namespace Microsoft::WRL;


class D3D12Render
{
protected:
    HWND mhMainWnd;//´°¿Ú¾ä±ú
    GameTimer gameTimer;//¼ÆÊ±Æ÷

    ComPtr<IDXGIFactory4> mdxgiFactory;

    ComPtr<ID3D12Device> md3dDevice;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12CommandQueue> mCommandQueue;

    ComPtr<IDXGISwapChain> mSwapChain;
    static const int SwapChainBufferCount = 2;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    int mCurrBackBuffer = 0;
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];

    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    std::unique_ptr<UploadBuffer<ObjectConsts>> mObjectCB;


    int mClientWidth = 800;
    int mClientHeight = 600;
    bool m4xMsaaState = false;
    UINT m4xMsaaQuality = 0;


    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    Camera mCamera;

    DirectX::XMFLOAT4X4 mWorld = MyMath::identity4x4();
    DirectX::XMFLOAT4X4 mView = MyMath::identity4x4();
    DirectX::XMFLOAT4X4 mProj = MyMath::identity4x4();

    ComPtr<ID3D12RootSignature> mRootSignature;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::unique_ptr<MeshGeometry> mMeshGeo = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;



public:
    D3D12Render(HWND wid,int width = 800,int height = 600) 
        :mhMainWnd(wid), mClientWidth(width), mClientHeight(height) {

    }
protected:

    void enableDebug();
    bool initDirect3D();
    void createDevice();
    void checkSupport();
    void checkMSAA();
    void checkTearing();
    void getDescriptorSize();

    void createCommandObjects();
    void createFence();
    void flushCommandQueue();

    ID3D12Resource* currentBackBuffer()const;
    void updateCurrBackBuffer();
    D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE constBufferView()const;
    
    void createSwapChain();
    void resizeSwapChain();

    virtual void createDescriptorHeaps();
    void createRtvHeap();
    void createDsvHeap();
    void createCbvHeap();

    void createDescriptor();
    void createRtvDescriptor();
    void createDsvDescriptor();
    void createCbvDescriptor();

    Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultBufferFrom(
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
   

    void setViewPort();
    void setScissorRect();

    void executeCommand();

    void createRootSignature();
    void createShadersAndInputLayout();
    void createMeshGeometry();
    void createPSO();

    virtual void onResize();
    virtual void updateData();
    virtual void draw();
    
    float getAspectRatio()const;
    void updateProjMatrix();
    void updateMVP();
};

