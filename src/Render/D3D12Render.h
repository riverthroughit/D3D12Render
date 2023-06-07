#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include"Utils/GameTimer.h"
#include"D3D12/D3D12RHI.h"
#include"Math/Math.h"
#include"Scene/Camera.h"
#include"PSO.h"
#include"RenderTarget.h"
#include<memory>



// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


using namespace Microsoft::WRL;


class D3D12Render
{
protected:
    HWND mhMainWnd;//´°¿Ú¾ä±ú
    bool beInitialized;
    int mClientWidth = 800;
    int mClientHeight = 600;

    std::unique_ptr<D3D12RHI> mD3D12RHI = nullptr;
    ID3D12Device* mD3DDevice;
    ID3D12GraphicsCommandList* mCommandList;

    InputLayoutManager mInputLayoutManager;

    std::unique_ptr<GraphicsPSOManager> mGraphicsPSOManager;
    GraphicsPSODescriptor mDeferredLightingPSODescriptor;



    std::unique_ptr<Shader> mDeferredLightingShader = nullptr;


    Camera mCamera;

    const int mGBufferCount = 3;
    std::unique_ptr<RenderTarget2D> mGBufferBaseColor;
    std::unique_ptr<RenderTarget2D> mGBufferNormal;
    std::unique_ptr<RenderTarget2D> mGBufferWorldPos;

    std::unique_ptr<RenderTarget2D> mBackDepth = nullptr;

    D3D12TextureRef mColorTexture = nullptr;
    D3D12TextureRef mCacheColorTexture = nullptr;
    D3D12TextureRef mPrevColorTexture = nullptr;

    std::unique_ptr<D3D12ShaderResourceView> mTexture3DNullDescriptor = nullptr;
    std::unique_ptr<D3D12ShaderResourceView> mTexture2DNullDescriptor = nullptr;
    std::unique_ptr<D3D12ShaderResourceView> mTextureCubeNullDescriptor = nullptr;
    std::unique_ptr<D3D12ShaderResourceView> mStructuredBufferNullDescriptor = nullptr;

public:
    D3D12Render(HWND wid,int width = 800,int height = 600) 
        :mhMainWnd(wid), beInitialized(false), mClientWidth(width), mClientHeight(height) {

    }
protected:

    bool initialize();
    virtual void onResize(int width,int height);
    virtual void updateData();
    virtual void draw();
    
    bool isInitialize() {
        return beInitialized;
    }

    float getAspectRatio()const;
    //void updateProjMatrix();
    //void updateMVP();
    
private:
    void createRenderResource();
    
    void createGBuffers();
    void createBackDepth();
    void createColorTextures();

    void createNullDescriptors();
    void createMesh();
    void createInputLayouts();
    void createGlobalShaders();
    void createGlobalPSO();
};

