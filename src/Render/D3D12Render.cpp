#include "D3D12Render.h"

using namespace DirectX;

bool D3D12Render::initDirect3D()
{
    enableDebug();

    //创建图形基础结构工厂
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));
    
    createDevice();
    checkSupport();
    
    getDescriptorSize();
    createDescriptorHeaps();
       
    createCommandObjects();
    createFence();
    
    createSwapChain();

    createCbvDescriptor();

    createRootSignature();
    createShadersAndInputLayout();
    
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    //需要用到命令队列提交更改资源状态的命令 将默认堆的状态改变
    createMeshGeometry();  
    executeCommand();
    flushCommandQueue();

    createPSO();

    onResize();

    return true;
}

void D3D12Render::onResize() {
    assert(md3dDevice);
    assert(mSwapChain);
    assert(mDirectCmdListAlloc);

    flushCommandQueue();
    

    resizeSwapChain();
    createRtvDescriptor();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    //需用命令队列提交命令 更改深度模板堆的状态
    createDsvDescriptor();  
    executeCommand();
    flushCommandQueue();
    
    setViewPort();
    setScissorRect();
    //宽高比可能发生改变 投影矩阵也要变
    updateProjMatrix();
}

void D3D12Render::updateData() {
    updateMVP();
}

void D3D12Render::draw() {

    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    //注意要传入流水线状态对象的地址
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    //呈现到渲染
    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            currentBackBuffer(),//句柄
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    //清空当前缓冲区
    mCommandList->ClearRenderTargetView(
        currentBackBufferView(),
        DirectX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(
        depthStencilView(),	
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0, 0,
        nullptr);
    
    //设置视口和裁剪矩形
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    //开始渲染

    //将渲染目标视图和深度模板视图绑定到流水线
    mCommandList->OMSetRenderTargets(1, &currentBackBufferView(), true, &depthStencilView());
    
    //将常量缓冲区描述符堆绑定到流水线
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    //绑定根签名
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    //绑定描述符表
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    //绑定顶点 索引以及图元拓扑
    mCommandList->IASetVertexBuffers(0, 1, &mMeshGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&mMeshGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //根据索引绘制图形
    mCommandList->DrawIndexedInstanced(
        mMeshGeo->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);

    //渲染到呈现
    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            currentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));

    //提交到队列
    executeCommand();
    //也需在此处设置禁用垂直同步
    ThrowIfFailed(mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    updateCurrBackBuffer();

    //等待gpu
    flushCommandQueue();
}

void D3D12Render::enableDebug() {
#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif
}

void D3D12Render::createDevice() {
    HRESULT hardWareRes = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
    //软光栅
    if (FAILED(hardWareRes)) {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)));
    }
}

void D3D12Render::checkSupport() {
    checkMSAA();
    checkTearing();
}

void D3D12Render::checkMSAA() {
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
    msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaQualityLevels.SampleCount = 1;
    msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaQualityLevels.NumQualityLevels = 0;
    //msaaQualityLevels既是输入也是输出
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
    m4xMsaaQuality = msaaQualityLevels.NumQualityLevels;
    //大于0才支持
    assert(m4xMsaaQuality > 0);
}

void D3D12Render::checkTearing(){
    //https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays#variable-refresh-rate-displaysvsync-off
    ComPtr<IDXGIFactory5> factory5;
    //使用1.4的接口转换
    ThrowIfFailed(mdxgiFactory.As(&factory5));
    bool allowTearing = false;
    factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
    //assert(allowTearing);
}

void D3D12Render::getDescriptorSize() {
    mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12Render::createCommandObjects() {
    //分配器和列表
    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    ThrowIfFailed(md3dDevice->CreateCommandList(0,commandListType, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
   //队列
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = commandListType;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));
    //先关闭 因为在onResize中会进行reset
    mCommandList->Close();
}

void D3D12Render::createFence() {
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void D3D12Render::flushCommandQueue() {
    ++mCurrentFence;
    //往命令队列中添加一条命令使gpu端围栏值在执行该命令后变为对应值
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void D3D12Render::createSwapChain() {
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
   
    //注意此处 表示显示在qt窗口上
    sd.OutputWindow = mhMainWnd;
    
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    //注意此处开启允许撕裂 即禁用垂直同步
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    //注意 需要命令队列的指针
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, &mSwapChain));
}

void D3D12Render::createRtvHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
}

void D3D12Render::createDsvHeap(){
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void D3D12Render::createCbvHeap(){
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //着色器可见
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void D3D12Render::createDescriptorHeaps() {
    createRtvHeap();
    createDsvHeap();
    createCbvHeap();
}

ID3D12Resource* D3D12Render::currentBackBuffer()const {
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

void D3D12Render::updateCurrBackBuffer() {
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Render::currentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer,
        mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Render::depthStencilView()const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Render::constBufferView()const {
    return mCbvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3D12Render::createDescriptor() {
    createRtvDescriptor();
    createDsvDescriptor();
    createCbvDescriptor();
}

void D3D12Render::createRtvDescriptor(){
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < SwapChainBufferCount; ++i){
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        //将指针移动到下一个描述符开始处
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }
}

void D3D12Render::createDsvDescriptor(){
    //先初始化构造信息
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    //用构造信息在GPU堆中生成深度模板缓冲资源
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)));

    //将资源绑定至描述符 同createRtvDescriptor
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, depthStencilView());
    //设置资源转换屏障 将深度缓冲转换为可写状态
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void D3D12Render::createCbvDescriptor() {
    //创建管理常量缓冲区的对象 并用unique指针指向它
    mObjectCB = std::make_unique<UploadBuffer<ObjectConsts>>(md3dDevice.Get(), 1, true);
    //常量缓冲区中单个对象地大小
    UINT objConstByteSize = ToolForD12::AlignArbitrary(sizeof(ObjectConsts), CONSTS_ALIGNMENT);

    //描述常量缓冲区资源
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objConstByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = objConstByteSize;

    //将资源绑定到描述符
    md3dDevice->CreateConstantBufferView(
        &cbvDesc,
        constBufferView());
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Render::createDefaultBufferFrom(
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    //默认堆
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    //上传堆
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    //需要复制到默认堆中的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    //更改资源状态 并调用UpdateSubresources函数将资源从cpu内存复制到上传堆 再复制到默认堆
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    //uploadBuffer需等待命令列表中的对应命令执行完成后再释放
   
    return defaultBuffer;
}

void D3D12Render::executeCommand() {
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void D3D12Render::setViewPort() {
    //即为视口变换参数
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;
}

void D3D12Render::setScissorRect() {
    //裁剪区域外的物体直接不渲染 不会改变物体原本形状
    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

void D3D12Render::resizeSwapChain() {
    for (int i = 0; i < SwapChainBufferCount; ++i)
        mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();

    ThrowIfFailed(mSwapChain->ResizeBuffers(
        SwapChainBufferCount,
        mClientWidth, mClientHeight,
        mBackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));//注意此处模式要和创建时一致

    mCurrBackBuffer = 0;
}

void D3D12Render::createRootSignature() {

    //根参数
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    //将根参数设为描述符表 描述符表中仅有一个常量缓冲区描述符
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    //将该描述符表绑定到b0寄存器
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    //用根参数描述根签名
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //将根签名的描述序列化处理
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    //创建根签名
    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

void D3D12Render::createShadersAndInputLayout()
{

    mvsByteCode = ToolForD12::CompileShader(L"src/Shaders/color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = ToolForD12::CompileShader(L"src/Shaders/color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void D3D12Render::createMeshGeometry() {
   
    
    
    std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    std::array<std::uint16_t, 36> indices =
    {
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

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mMeshGeo = std::make_unique<MeshGeometry>();
    mMeshGeo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mMeshGeo->vertexBufferCPU));
    CopyMemory(mMeshGeo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mMeshGeo->indexBufferCPU));
    CopyMemory(mMeshGeo->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mMeshGeo->vertexBufferGPU = createDefaultBufferFrom(vertices.data(), 
        vbByteSize, mMeshGeo->vertexBufferUploader);

    mMeshGeo->indexBufferGPU = createDefaultBufferFrom(indices.data(), 
        ibByteSize, mMeshGeo->indexBufferUploader);

    mMeshGeo->vertexByteStride = sizeof(Vertex);
    mMeshGeo->vertexBufferByteSize = vbByteSize;
    mMeshGeo->indexFormat = DXGI_FORMAT_R16_UINT;
    mMeshGeo->indexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mMeshGeo->DrawArgs["box"] = submesh;
}

void D3D12Render::createPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
        mvsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
        mpsByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

float D3D12Render::getAspectRatio()const{
    return static_cast<float>(mClientWidth) / mClientHeight;
}

void D3D12Render::updateProjMatrix(){
    XMMATRIX P = XMMatrixPerspectiveFovLH(
        mCamera.getFovAngleY(), getAspectRatio(), mCamera.getNear(), mCamera.getFar());
    XMStoreFloat4x4(&mProj, P);
}

void D3D12Render::updateMVP() {
    XMMATRIX view = XMMatrixLookAtLH(mCamera.getPos(), mCamera.getTarget(), mCamera.getUp());
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    ObjectConsts objectConsts;
    XMStoreFloat4x4(&objectConsts.mvp, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(0, objectConsts);
}