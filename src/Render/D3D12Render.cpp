#include "D3D12Render.h"

using namespace DirectX;

bool D3D12Render::initDirect3D()
{
    enableDebug();

    //����ͼ�λ����ṹ����
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
    //��Ҫ�õ���������ύ������Դ״̬������ ��Ĭ�϶ѵ�״̬�ı�
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
    //������������ύ���� �������ģ��ѵ�״̬
    createDsvDescriptor();  
    executeCommand();
    flushCommandQueue();
    
    setViewPort();
    setScissorRect();
    //��߱ȿ��ܷ����ı� ͶӰ����ҲҪ��
    updateProjMatrix();
}

void D3D12Render::updateData() {
    updateMVP();
}

void D3D12Render::draw() {

    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    //ע��Ҫ������ˮ��״̬����ĵ�ַ
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    //���ֵ���Ⱦ
    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            currentBackBuffer(),//���
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    //��յ�ǰ������
    mCommandList->ClearRenderTargetView(
        currentBackBufferView(),
        DirectX::Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(
        depthStencilView(),	
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0, 0,
        nullptr);
    
    //�����ӿںͲü�����
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    //��ʼ��Ⱦ

    //����ȾĿ����ͼ�����ģ����ͼ�󶨵���ˮ��
    mCommandList->OMSetRenderTargets(1, &currentBackBufferView(), true, &depthStencilView());
    
    //�������������������Ѱ󶨵���ˮ��
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    //�󶨸�ǩ��
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    //����������
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    //�󶨶��� �����Լ�ͼԪ����
    mCommandList->IASetVertexBuffers(0, 1, &mMeshGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&mMeshGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //������������ͼ��
    mCommandList->DrawIndexedInstanced(
        mMeshGeo->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);

    //��Ⱦ������
    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            currentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));

    //�ύ������
    executeCommand();
    //Ҳ���ڴ˴����ý��ô�ֱͬ��
    ThrowIfFailed(mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    updateCurrBackBuffer();

    //�ȴ�gpu
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
    //���դ
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
    //msaaQualityLevels��������Ҳ�����
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevels, sizeof(msaaQualityLevels)));
    m4xMsaaQuality = msaaQualityLevels.NumQualityLevels;
    //����0��֧��
    assert(m4xMsaaQuality > 0);
}

void D3D12Render::checkTearing(){
    //https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays#variable-refresh-rate-displaysvsync-off
    ComPtr<IDXGIFactory5> factory5;
    //ʹ��1.4�Ľӿ�ת��
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
    //���������б�
    D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    ThrowIfFailed(md3dDevice->CreateCommandList(0,commandListType, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
   //����
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = commandListType;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));
    //�ȹر� ��Ϊ��onResize�л����reset
    mCommandList->Close();
}

void D3D12Render::createFence() {
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void D3D12Render::flushCommandQueue() {
    ++mCurrentFence;
    //��������������һ������ʹgpu��Χ��ֵ��ִ�и�������Ϊ��Ӧֵ
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
   
    //ע��˴� ��ʾ��ʾ��qt������
    sd.OutputWindow = mhMainWnd;
    
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    //ע��˴���������˺�� �����ô�ֱͬ��
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    //ע�� ��Ҫ������е�ָ��
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
    //��ɫ���ɼ�
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
        //��ָ���ƶ�����һ����������ʼ��
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }
}

void D3D12Render::createDsvDescriptor(){
    //�ȳ�ʼ��������Ϣ
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

    //�ù�����Ϣ��GPU�����������ģ�建����Դ
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&mDepthStencilBuffer)));

    //����Դ���������� ͬcreateRtvDescriptor
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, depthStencilView());
    //������Դת������ ����Ȼ���ת��Ϊ��д״̬
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void D3D12Render::createCbvDescriptor() {
    //�����������������Ķ��� ����uniqueָ��ָ����
    mObjectCB = std::make_unique<UploadBuffer<ObjectConsts>>(md3dDevice.Get(), 1, true);
    //�����������е�������ش�С
    UINT objConstByteSize = ToolForD12::AlignArbitrary(sizeof(ObjectConsts), CONSTS_ALIGNMENT);

    //����������������Դ
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objConstByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = objConstByteSize;

    //����Դ�󶨵�������
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

    //Ĭ�϶�
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    //�ϴ���
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    //��Ҫ���Ƶ�Ĭ�϶��е�����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    //������Դ״̬ ������UpdateSubresources��������Դ��cpu�ڴ渴�Ƶ��ϴ��� �ٸ��Ƶ�Ĭ�϶�
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    //uploadBuffer��ȴ������б��еĶ�Ӧ����ִ����ɺ����ͷ�
   
    return defaultBuffer;
}

void D3D12Render::executeCommand() {
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void D3D12Render::setViewPort() {
    //��Ϊ�ӿڱ任����
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;
}

void D3D12Render::setScissorRect() {
    //�ü������������ֱ�Ӳ���Ⱦ ����ı�����ԭ����״
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
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));//ע��˴�ģʽҪ�ʹ���ʱһ��

    mCurrBackBuffer = 0;
}

void D3D12Render::createRootSignature() {

    //������
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    //����������Ϊ�������� ���������н���һ������������������
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    //������������󶨵�b0�Ĵ���
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    //�ø�����������ǩ��
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //����ǩ�����������л�����
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    //������ǩ��
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