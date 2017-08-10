//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "d3dx12affinity.h"
#include "CD3DX12Utils.h"
#include "CD3DX12Settings.h"



#ifdef D3DX12_AFFINITY_CALL0
#undef D3DX12_AFFINITY_CALL0
#undef D3DX12_AFFINITY_CALL
#endif

#ifndef D3DX12AFFINITY_ENABLE_DEVICE_DEBUG
#define D3DX12_AFFINITY_CALL0(x)
#define D3DX12_AFFINITY_CALL(x, ...)
#else
#include "d3dx12affinity_debug.h"
#define D3DX12_AFFINITY_CALL0(x) D3DX12_AFFINITY_DOCALL0(x)
#define D3DX12_AFFINITY_CALL(x, ...) D3DX12_AFFINITY_DOCALL(x, __VA_ARGS__)
#endif

#pragma region Statics
UINT CD3DX12AffinityDeviceStatics::g_CachedNodeCount = 0;
UINT CD3DX12AffinityDeviceStatics::g_CachedNodeMask = 0;
UINT CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask = 0;
UINT CD3DX12AffinityDeviceStatics::g_CachedActualNodeCount = 0;

//thread_local UINT g_ActiveNodeIndex = 0xFFFFFFFF;
thread_local UINT g_ActiveNodeIndex = 0;


UINT CD3DX12AffinityDeviceStatics::GetActiveNodeID()
{
#ifdef D3DX12AFFINITY_EC_VALIDATE_ACTIVE_ID
	if (g_ActiveNodeIndex == 0xFFFFFFFF)
	{
		D3DX12AFFINITY_LOG("Returning invalid node index!");
		D3DX12AFFINITY_BREAK;
	}
#endif

	return g_ActiveNodeIndex;
}

UINT CD3DX12AffinityDeviceStatics::GetActiveNodeID_RAW()
{
	return g_ActiveNodeIndex;
}

void CD3DX12AffinityDeviceStatics::SetActiveNodeID(UINT id)
{
	g_ActiveNodeIndex = id;
}
#pragma endregion

#pragma region Internals

static bool operator < (D3D12_CPU_DESCRIPTOR_HANDLE a, D3D12_CPU_DESCRIPTOR_HANDLE b) { return a.ptr < b.ptr; }

struct Internal_CD3DX12AffinityDevice
{
#ifdef D3DX12AFFINITY_EC_VALIDATE_DESCRIPTORS
	std::mutex DescriptorResourceMapLock;
	std::map<D3D12_CPU_DESCRIPTOR_HANDLE, CD3DX12AffinityResource*> DescriptorResourceMap;
#endif
};


void CD3DX12AffinityDevice::DbgValidateDescriptorSet(D3D12_CPU_DESCRIPTOR_HANDLE desc, CD3DX12AffinityResource* res)
{
#ifdef D3DX12AFFINITY_EC_VALIDATE_DESCRIPTORS
	mInternal->DescriptorResourceMapLock.lock();
	mInternal->DescriptorResourceMap[desc] = res;
	mInternal->DescriptorResourceMapLock.unlock();
#endif
}
CD3DX12AffinityResource* CD3DX12AffinityDevice::DbgValidateDescriptorGet(D3D12_CPU_DESCRIPTOR_HANDLE desc)
{
#ifdef D3DX12AFFINITY_EC_VALIDATE_DESCRIPTORS
	CD3DX12AffinityResource* res;
	mInternal->DescriptorResourceMapLock.lock();
	auto it = mInternal->DescriptorResourceMap.find(desc);
	res = (it != mInternal->DescriptorResourceMap.end()) ? it->second : nullptr;
	mInternal->DescriptorResourceMapLock.unlock();
	return res;
#else
	return nullptr;
#endif
}


#pragma endregion

void STDMETHODCALLTYPE CD3DX12AffinityDevice::SetAffinity(UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::SetAffinity, AffinityMask);

    CD3DX12AffinityObject::SetAffinity(AffinityMask);
    UpdateActiveDevices();
}

void CD3DX12AffinityDevice::UpdateActiveDevices()
{
	D3DX12_AFFINITY_CALL0(ID3D12Device::UpdateActiveDevices);

    mNumActiveDevices = 0;
    for (int i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES; i++)
    {
        if (mDevices[i])
        {
            mNumActiveDevices++;
        }
        else
        {
            break;
        }
    }
}

UINT STDMETHODCALLTYPE CD3DX12AffinityDevice::GetNodeCount(void)
{
	return CD3DX12AffinityDeviceStatics::g_CachedNodeCount;
}

UINT CD3DX12AffinityDevice::LDAAllNodeMasks()
{
    return ((1 << mLDANodeCount) - 1) & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateCommandQueue(
    const D3D12_COMMAND_QUEUE_DESC* pDesc,
    REFIID riid,
    void** ppCommandQueue,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateCommandQueue, pDesc, riid, ppCommandQueue, AffinityMask);
    D3D12_COMMAND_QUEUE_DESC Desc = *pDesc;

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    std::vector<ID3D12CommandQueue*> Queues;
    Queues.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12CommandQueue* Queue = nullptr;

                Desc.NodeMask = AffinityIndexToNodeMask(i);
                HRESULT hr = Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&Queue));
                if (S_OK == hr)
                {
                    Queues[i] = Queue;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityCommandQueue* CommandQueue = new CD3DX12AffinityCommandQueue(this, &(Queues[0]), (UINT)Queues.size());
    (*ppCommandQueue) = CommandQueue;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE type,
    REFIID riid,
    void** ppCommandAllocator,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateCommandAllocator, type, riid, ppCommandAllocator, AffinityMask);

    std::vector<ID3D12CommandAllocator*> Allocators;
    Allocators.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12CommandAllocator* Allocator = nullptr;
                HRESULT hr = Device->CreateCommandAllocator(type, IID_PPV_ARGS(&Allocator));
                if (S_OK == hr)
                {
                    Allocators[i] = Allocator;
                }
                else
                {
                    return hr;
                }
            }
        }
    }

    CD3DX12AffinityCommandAllocator* CommandAllocator = new CD3DX12AffinityCommandAllocator(this, &(Allocators[0]), (UINT)Allocators.size(), AffinityMask == 0);
    (*ppCommandAllocator) = CommandAllocator;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateGraphicsPipelineState(
    const D3DX12_AFFINITY_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateGraphicsPipelineState, pDesc, riid, ppPipelineState, AffinityMask);

    CD3DX12AffinityRootSignature* AffinityRootSignature = static_cast<CD3DX12AffinityRootSignature*>(pDesc->pRootSignature);

    std::vector<ID3D12PipelineState*> PipelineStates;
    PipelineStates.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        //
        D3D12_GRAPHICS_PIPELINE_STATE_DESC ActualDescriptor;
        ActualDescriptor = pDesc->ToD3D12();
        ActualDescriptor.pRootSignature = AffinityRootSignature->mRootSignatures[0];
        ActualDescriptor.NodeMask = LDAAllNodeMasks();

        ID3D12PipelineState* PipelineState = nullptr;
        HRESULT const hr = Device->CreateGraphicsPipelineState(&ActualDescriptor, IID_PPV_ARGS(&PipelineState));
        if (S_OK == hr)
        {
            for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
            {
                if (((1 << i) & EffectiveAffinityMask) != 0)
                {
                    PipelineState->AddRef();
                    PipelineStates[i] = PipelineState;

                }
            }
            PipelineState->Release();
        }
        else
        {
            return hr;
        }
    }
    CD3DX12AffinityPipelineState* AffinityPipelineState = new CD3DX12AffinityPipelineState(this, &(PipelineStates[0]), (UINT)PipelineStates.size());
    (*ppPipelineState) = AffinityPipelineState;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateComputePipelineState(
    const D3DX12_AFFINITY_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateComputePipelineState, pDesc, riid, ppPipelineState, AffinityMask);

    CD3DX12AffinityRootSignature* AffinityRootSignature = static_cast<CD3DX12AffinityRootSignature*>(pDesc->pRootSignature);

    std::vector<ID3D12PipelineState*> PipelineStates;
    PipelineStates.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];

        D3D12_COMPUTE_PIPELINE_STATE_DESC ActualDescriptor;
        ActualDescriptor = pDesc->ToD3D12();
        ActualDescriptor.pRootSignature = AffinityRootSignature->mRootSignatures[0];
        ActualDescriptor.NodeMask = LDAAllNodeMasks();

        ID3D12PipelineState* PipelineState = nullptr;
        HRESULT const hr = Device->CreateComputePipelineState(&ActualDescriptor, IID_PPV_ARGS(&PipelineState));
        if (S_OK == hr)
        {
            for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
            {
                if (((1 << i) & EffectiveAffinityMask) != 0)
                {
                    PipelineState->AddRef();
                    PipelineStates[i] = PipelineState;
                }
            }
            PipelineState->Release();
        }
        else
        {
            return hr;
        }
    }

    CD3DX12AffinityPipelineState* AffinityPipelineState = new CD3DX12AffinityPipelineState(this, &(PipelineStates[0]), (UINT)PipelineStates.size());
    (*ppPipelineState) = AffinityPipelineState;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateCommandList(
    UINT nodeMask,
    D3D12_COMMAND_LIST_TYPE type,
    CD3DX12AffinityCommandAllocator* pCommandAllocator,
    CD3DX12AffinityPipelineState* pInitialState,
    REFIID riid,
    void** ppCommandList,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateCommandList, nodeMask, type, pCommandAllocator, pInitialState, riid, ppCommandList, AffinityMask);

    std::vector<ID3D12GraphicsCommandList*> CommandLists;
    CommandLists.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    CD3DX12AffinityCommandAllocator* WrappedAllocator = static_cast<CD3DX12AffinityCommandAllocator*>(pCommandAllocator);
    CD3DX12AffinityPipelineState* WrappedState = static_cast<CD3DX12AffinityPipelineState*>(pInitialState);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();

    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12GraphicsCommandList* CommandList = nullptr;
                HRESULT hr = Device->CreateCommandList(AffinityIndexToNodeMask(i), type,
                    WrappedAllocator->GetChildObject(i),
                    WrappedState ? WrappedState->mPipelineStates[i] : nullptr, IID_PPV_ARGS(&CommandList));

                if (S_OK == hr)
                {
#if !D3DXAFFINITY_ALWAYS_RESET_ALL_COMMAND_LISTS
                    //When a new commandlist is created, it is in active state
                    //the next time the active node is changed the commnadlist will be in closed state
                    //So make sure all commandlists on non-active nodes are closed for next reuse
                    if (i != GetActiveNodeIndex() && AffinityMask == 0)
                    {
                        CommandList->Close();
                    }
#endif
                    CommandLists[i] = CommandList;
                }
                else
                {
                    return hr;
                }
            }
        }
    }


    CD3DX12AffinityGraphicsCommandList* CommandList = new CD3DX12AffinityGraphicsCommandList(this, &(CommandLists[0]), (UINT)CommandLists.size(), AffinityMask == 0);
    (*ppCommandList) = CommandList;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CheckFeatureSupport(
    D3D12_FEATURE Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize,
    UINT AffinityIndex)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CheckFeatureSupport, Feature, pFeatureSupportData, FeatureSupportDataSize, AffinityIndex);

    // Return feature support for device 0 and hope that that's consistent!
    return mDevices[AffinityIndex]->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateDescriptorHeap(
    const D3D12_DESCRIPTOR_HEAP_DESC* pDescriptorHeapDesc,
    REFIID riid,
    void** ppvHeap,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateDescriptorHeap, pDescriptorHeapDesc, riid, ppvHeap, AffinityMask);

    D3D12_DESCRIPTOR_HEAP_DESC Desc = *pDescriptorHeapDesc;

    std::vector<ID3D12DescriptorHeap*> Heaps;
    Heaps.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12DescriptorHeap* Heap = nullptr;

                Desc.NodeMask = AffinityIndexToNodeMask(i);
                HRESULT hr = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
                if (S_OK == hr)
                {
                    Heaps[i] = Heap;
                }
                else
                {
                    return hr;
                }
            }
        }
    }

    CD3DX12AffinityDescriptorHeap* DescriptorHeap = new CD3DX12AffinityDescriptorHeap(this, &(Heaps[0]), (UINT)Heaps.size());
    DescriptorHeap->mNumDescriptors = pDescriptorHeapDesc->NumDescriptors;
    if (D3DXAFFINITY_GET_NODE_COUNT > 1)
    {
        DescriptorHeap->mCPUHeapStart = new UINT64[pDescriptorHeapDesc->NumDescriptors * D3DXAFFINITY_GET_NODE_COUNT]();
        DescriptorHeap->mGPUHeapStart = new UINT64[pDescriptorHeapDesc->NumDescriptors * D3DXAFFINITY_GET_NODE_COUNT]();

        D3DX12AFFINITY_LOG("Allocated %u spots in heap array\n", pDescriptorHeapDesc->NumDescriptors * D3DXAFFINITY_GET_NODE_COUNT);

        DescriptorHeap->InitDescriptorHandles(pDescriptorHeapDesc->Type);
    }
    (*ppvHeap) = DescriptorHeap;

    return S_OK;
}

UINT STDMETHODCALLTYPE CD3DX12AffinityDevice::GetDescriptorHandleIncrementSize(
    D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType,
    UINT AffinityMask)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetDescriptorHandleIncrementSize, DescriptorHeapType, AffinityMask);

    if (D3DXAFFINITY_GET_NODE_COUNT == 1)
    {
        return mDevices[0]->GetDescriptorHandleIncrementSize(DescriptorHeapType);
    }
    return sizeof(UINT64) * D3DXAFFINITY_GET_NODE_COUNT;
}

UINT STDMETHODCALLTYPE CD3DX12AffinityDevice::GetActiveDescriptorHandleIncrementSize(
    D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType,
    UINT AffinityIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetActiveDescriptorHandleIncrementSize, DescriptorHeapType, AffinityIndex);

    UINT HandleIncrementSize = 0;
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        HandleIncrementSize = mDevices[0]->GetDescriptorHandleIncrementSize(DescriptorHeapType);
    }
    return HandleIncrementSize;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateRootSignature(
    UINT nodeMask,
    const void* pBlobWithRootSignature,
    SIZE_T blobLengthInBytes,
    REFIID riid,
    void** ppvRootSignature,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateRootSignature, nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, ppvRootSignature, AffinityMask);

    std::vector<ID3D12RootSignature*> Signatures;
    Signatures.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        ID3D12RootSignature* Signature = nullptr;
        UINT MaskToUse = LDAAllNodeMasks();
        HRESULT hr = Device->CreateRootSignature(MaskToUse, pBlobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(&Signature));
        if (S_OK == hr)
        {
            for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
            {
                if (((1 << i) & EffectiveAffinityMask) != 0)
                {
                    Signature->AddRef();
                    Signatures[i] = Signature;
                }
            }
            Signature->Release();
        }
        else
        {
            return hr;
        }
    }
    CD3DX12AffinityRootSignature* Signature = new CD3DX12AffinityRootSignature(this, &(Signatures[0]), (UINT)Signatures.size());
    (*ppvRootSignature) = Signature;

    return S_OK;
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateConstantBufferView(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateConstantBufferView, pDesc, DestDescriptor, AffinityMask);

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);

                D3D12_CONSTANT_BUFFER_VIEW_DESC ActualDesc = *pDesc;
                ActualDesc.BufferLocation = GetGPUVirtualAddress(pDesc->BufferLocation, i);

                Device->CreateConstantBufferView(&ActualDesc, ActualDestDescriptor);
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateShaderResourceView(
    CD3DX12AffinityResource* pResource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
    CreateShaderResourceViewWithAffinity(pResource, pDesc, DestDescriptor, EAffinityMask::AllNodes);
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateShaderResourceViewWithAffinity(
    CD3DX12AffinityResource* pResource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateShaderResourceViewWithAffinity, pResource, pDesc, DestDescriptor, AffinityMask);

    CD3DX12AffinityResource* AffinityResource = nullptr;
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (pResource)
    {
        AffinityResource = static_cast<CD3DX12AffinityResource*>(pResource);
    }
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);

                Device->CreateShaderResourceView(AffinityResource ? AffinityResource->mResources[i] : nullptr, pDesc, ActualDestDescriptor);

            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateUnorderedAccessView(
    CD3DX12AffinityResource* pResource,
    CD3DX12AffinityResource* pCounterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
    CreateUnorderedAccessViewWithAffinity(pResource, pCounterResource, pDesc, DestDescriptor, EAffinityMask::AllNodes);
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateUnorderedAccessViewWithAffinity(
    CD3DX12AffinityResource* pResource,
    CD3DX12AffinityResource* pCounterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateUnorderedAccessViewWithAffinity, pResource, pCounterResource, pDesc, DestDescriptor, AffinityMask);

    CD3DX12AffinityResource* AffinityResource = nullptr;
    if (pResource)
    {
        AffinityResource = static_cast<CD3DX12AffinityResource*>(pResource);
    }
    CD3DX12AffinityResource* AffinityCounterResource = nullptr;
    if (pCounterResource)
    {
        AffinityCounterResource = static_cast<CD3DX12AffinityResource*>(pCounterResource);
    }

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);

                Device->CreateUnorderedAccessView(
                    AffinityResource ? AffinityResource->mResources[i] : nullptr,
                    AffinityCounterResource ? AffinityCounterResource->mResources[i] : nullptr,
                    pDesc, ActualDestDescriptor);
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateRenderTargetView(
    CD3DX12AffinityResource* pResource,
    const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
    CreateRenderTargetViewWithAffinity(pResource, pDesc, DestDescriptor, EAffinityMask::AllNodes);
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateRenderTargetViewWithAffinity(
    CD3DX12AffinityResource* pResource,
    const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateRenderTargetView, pResource, pDesc, DestDescriptor, AffinityMask);

    CD3DX12AffinityResource* AffinityResource = nullptr;
    if (pResource)
    {
        AffinityResource = static_cast<CD3DX12AffinityResource*>(pResource);
    }

#ifdef D3DX12AFFINITY_EC_VALIDATE_DESCRIPTORS
	DbgValidateDescriptorSet(DestDescriptor, AffinityResource);
#endif

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);

                ID3D12Resource* Resource = nullptr;
				if (AffinityResource)
				{
					Resource = AffinityResource->mResources[i];
				}

#ifdef D3DX12AFFINITY_ENABLE_RTV_DEBUG
				D3DX12AFFINITY_LOG("CreateRenderTargetViewWithAffinity %d %llX %llX %llX", i, DestDescriptor.ptr, ActualDestDescriptor.ptr, (unsigned long long)Resource);
#endif

				if (pDesc)
					Device->CreateRenderTargetView(Resource, pDesc, ActualDestDescriptor);
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateDepthStencilView(
    CD3DX12AffinityResource* pResource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
    CreateDepthStencilViewWithAffinity(pResource, pDesc, DestDescriptor, EAffinityMask::AllNodes);
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateDepthStencilViewWithAffinity(
    CD3DX12AffinityResource* pResource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateDepthStencilViewWithAffinity, pResource, pDesc, DestDescriptor, AffinityMask);

    CD3DX12AffinityResource* AffinityResource = nullptr;
    if (pResource)
    {
        AffinityResource = static_cast<CD3DX12AffinityResource*>(pResource);
    }

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);

                ID3D12Resource* Resource = nullptr;
                if (AffinityResource)
                {
                    Resource = AffinityResource->mResources[i];
                }
                Device->CreateDepthStencilView(Resource, pDesc, ActualDestDescriptor);
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateSampler(
    const D3D12_SAMPLER_DESC* pDesc,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateSampler, pDesc, DestDescriptor, AffinityMask);

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptor, i);
                Device->CreateSampler(pDesc, ActualDestDescriptor);
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CopyDescriptors(
    UINT NumDestDescriptorRanges,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts,
    const UINT* pDestDescriptorRangeSizes,
    UINT NumSrcDescriptorRanges,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts,
    const UINT* pSrcDescriptorRangeSizes,
    D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CopyDescriptors, NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges, pSrcDescriptorRangeSizes, DescriptorHeapsType, AffinityMask);

    if (NumDestDescriptorRanges == 1 && NumSrcDescriptorRanges == 1)
    {
        return CopyDescriptorsOne(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges,
            pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType, AffinityMask);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE* ActualDestDescriptorRangeStarts = new D3D12_CPU_DESCRIPTOR_HANDLE[NumDestDescriptorRanges];
    D3D12_CPU_DESCRIPTOR_HANDLE* ActualSrcDescriptorRangeStarts = new D3D12_CPU_DESCRIPTOR_HANDLE[NumSrcDescriptorRanges];
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {

                {
                    for (UINT t = 0; t < NumDestDescriptorRanges; ++t)
                    {
                        ActualDestDescriptorRangeStarts[t] = GetCPUHeapPointer(pDestDescriptorRangeStarts[t], i);
                    }
                    for (UINT t = 0; t < NumSrcDescriptorRanges; ++t)
                    {
                        ActualSrcDescriptorRangeStarts[t] = GetCPUHeapPointer(pSrcDescriptorRangeStarts[t], i);
                    }

                    Device->CopyDescriptors(
                        NumDestDescriptorRanges, ActualDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                        NumSrcDescriptorRanges, ActualSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                        DescriptorHeapsType);
                }
            }
        }
    }

    delete[] ActualDestDescriptorRangeStarts;
    delete[] ActualSrcDescriptorRangeStarts;

}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CopyDescriptorsOne(
    UINT NumDestDescriptorRanges,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts,
    const UINT* pDestDescriptorRangeSizes,
    UINT NumSrcDescriptorRanges,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts,
    const UINT* pSrcDescriptorRangeSizes,
    D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CopyDescriptorsOne, NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges, pSrcDescriptorRangeSizes, DescriptorHeapsType, AffinityMask);

    UINT ActiveNodeIndex = GetActiveNodeIndex();

    D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptorRangeStarts[1];
    D3D12_CPU_DESCRIPTOR_HANDLE ActualSrcDescriptorRangeStarts[1];
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                {
                    ActualDestDescriptorRangeStarts[0] = GetCPUHeapPointer(pDestDescriptorRangeStarts[0], i);
                    ActualSrcDescriptorRangeStarts[0] = GetCPUHeapPointer(pSrcDescriptorRangeStarts[0], i);

                    Device->CopyDescriptors(
                        NumDestDescriptorRanges, ActualDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                        NumSrcDescriptorRanges, ActualSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                        DescriptorHeapsType);
                }
            }
        }
    }
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::CopyDescriptorsSimple(
    UINT NumDescriptors,
    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
    D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
    D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CopyDescriptorsSimple, NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType, AffinityMask);

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE ActualDestDescriptor = GetCPUHeapPointer(DestDescriptorRangeStart, i);
                D3D12_CPU_DESCRIPTOR_HANDLE ActualSrcDescriptor = GetCPUHeapPointer(SrcDescriptorRangeStart, i);

                Device->CopyDescriptorsSimple(NumDescriptors, ActualDestDescriptor, ActualSrcDescriptor, DescriptorHeapsType);
            }
        }
    }
}

D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE CD3DX12AffinityDevice::GetResourceAllocationInfo(
    UINT visibleMask,
    UINT numResourceDescs,
    const D3D12_RESOURCE_DESC* pResourceDescs,
    UINT AffinityIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetResourceAllocationInfo, visibleMask, numResourceDescs, pResourceDescs, AffinityIndex);

    return mDevices[AffinityIndex]->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs);
}

D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE CD3DX12AffinityDevice::GetCustomHeapProperties(
    UINT nodeMask,
    D3D12_HEAP_TYPE heapType,
    UINT AffinityIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetResourceAllocationInfo, nodeMask, heapType, AffinityIndex);

    return mDevices[AffinityIndex]->GetCustomHeapProperties(nodeMask, heapType);
}

UINT64 GetBufferSizeForResource(
    ID3D12Resource* pResource)
{
    D3D12_RESOURCE_DESC Desc = pResource->GetDesc();
    UINT64 Size = 0;
    switch (Desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
    {
        DEBUG_ASSERT(Desc.Height == 1);
        Size = Desc.Width;
    }
    break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
    {
        ID3D12Device* pDevice;
        pResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
        auto info = pDevice->GetResourceAllocationInfo(0, 1, &Desc);
        pDevice->Release();

        Size = info.SizeInBytes;
    }
    break;
    default:
    case D3D12_RESOURCE_DIMENSION_UNKNOWN:
    {
        DEBUG_ASSERT(false);
    }
    break;
    }
    return Size;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateCommittedResource(
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const D3D12_RESOURCE_DESC* pResourceDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateCommittedResource, pHeapProperties, HeapFlags, pResourceDesc, InitialResourceState, pOptimizedClearValue, ppvResource, AffinityMask);

    D3D12_HEAP_PROPERTIES Properties = *(const_cast<D3D12_HEAP_PROPERTIES*>(pHeapProperties));

	// [LSIKKES] - Honor Properties.VisibleNodeMask
	bool VisibleToAll = (Properties.VisibleNodeMask & LDAAllNodeMasks()) == LDAAllNodeMasks();
	// [/LSIKKES]

    ID3D12Resource* Resources[D3DX12_MAX_ACTIVE_NODES] = {};
    ID3D12Heap* Heaps[D3DX12_MAX_ACTIVE_NODES] = {};
    ID3D12Heap** pHeaps = nullptr;
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        D3D12_HEAP_PROPERTIES heapProp = Device->GetCustomHeapProperties(0, pHeapProperties->Type);

        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                UINT nodeMask = AffinityIndexToNodeMask(i);
                Properties.CreationNodeMask = nodeMask & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
                Properties.VisibleNodeMask = LDAAllNodeMasks() & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
                if (heapProp.MemoryPoolPreference == D3D12_MEMORY_POOL_L0)
                {
                    // System memory, don't create the resource more than once or manager syncronization.
                    if (i == 0)
                    {
                        HRESULT hr = Device->CreateCommittedResource(&Properties, HeapFlags, pResourceDesc, InitialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&Resources[i]));
                        RETURN_IF_FAILED(hr);
                    }
                    else
                    {
                        Resources[i] = Resources[i - 1];
                        Resources[i]->AddRef();
                    }

                }
                else
                {
					// [LSIKKES] - Honor Properties.VisibleNodeMask
					if(VisibleToAll == false)
						Properties.VisibleNodeMask = nodeMask & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
					// [/LSIKKES]

#if D3DXAFFINITY_TILE_MAPPING_GPUVA
                    if (D3DXAFFINITY_GET_NODE_COUNT > 1 &&
                        pResourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
                    {
                        UINT64 Width = (pResourceDesc->Width + D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES - 1)
                            & ~(D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES - 1);
                        // Video memory buffers - allocate heaps on each GPU and a single reserved resource so we don't have to remap GPUVA.
                        D3D12_HEAP_DESC HeapDesc = { Width, Properties, pResourceDesc->Alignment, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS };
                        HRESULT hr = Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&Heaps[i]));
                        RETURN_IF_FAILED(hr);
                    }
                    else
#endif
                    {
                        HRESULT hr = Device->CreateCommittedResource(&Properties, HeapFlags, pResourceDesc, InitialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&Resources[i]));
                        RETURN_IF_FAILED(hr);
                    }
                }
            }
        }
#if D3DXAFFINITY_TILE_MAPPING_GPUVA
        if (D3DXAFFINITY_GET_NODE_COUNT > 1 &&
            pResourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
            heapProp.MemoryPoolPreference != D3D12_MEMORY_POOL_L0)
        {
            HRESULT hr = Device->CreateReservedResource(pResourceDesc, InitialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&Resources[0]));
            RETURN_IF_FAILED(hr);

            pHeaps = Heaps;

            for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
            {
                if (((1 << i) & EffectiveAffinityMask) != 0)
                {

                    if (i > 0)
                    {
                        Resources[i] = Resources[0];
                        Resources[i]->AddRef();
                    }

                    D3D12_TILED_RESOURCE_COORDINATE Coord = {};
                    D3D12_TILE_REGION_SIZE Region = { (UINT)(((pResourceDesc->Width - 1ull) / D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES) + 1) };
                    D3D12_TILE_RANGE_FLAGS RangeFlags = D3D12_TILE_RANGE_FLAG_NONE;
                    UINT RangeStart = 0;

                    UINT HeapIndex = i;
#if FORCE_REMOTE_TILE_MAPPING_GPUVA
                    HeapIndex = AffinityIndices[(Counter + 1) % IndicesCount];
#endif

                    mSyncCommandQueues[i]->UpdateTileMappings(Resources[i], 1, &Coord, &Region, Heaps[HeapIndex], 1, &RangeFlags, &RangeStart, &Region.NumTiles, D3D12_TILE_MAPPING_FLAG_NO_HAZARD);
                }

                ID3D12Fence* pFence = nullptr;
                RETURN_IF_FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
                HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
                {
                    if (((1 << i) & EffectiveAffinityMask) != 0)
                    {
                        mSyncCommandQueues[i]->Signal(pFence, i + 1);
                        pFence->SetEventOnCompletion(i + 1, hEvent);
                        WaitForSingleObject(hEvent, INFINITE);
                    }
                }
                pFence->Release();
            }
        }
#endif
    }

    CD3DX12AffinityResource* Resource = new CD3DX12AffinityResource(this, Resources, D3DXAFFINITY_GET_NODE_COUNT, pHeaps);
    Resource->mMappedReferenceCount = 0;
    Resource->mBufferSize = GetBufferSizeForResource(Resources[0]);
    Resource->mCPUPageProperty = mDevices[0]->GetCustomHeapProperties(0, pHeapProperties->Type).CPUPageProperty;
    {
        ReleaseLog(L"D3DX12AffinityLayer: Committed resource is not write combine, creating no shadow buffer.\n", Resource->mBufferSize);
        Resource->mShadowBuffer = nullptr;
    }

    (*ppvResource) = Resource;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateHeap(
    const D3D12_HEAP_DESC* pDesc,
    REFIID riid,
    void** ppvHeap,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateHeap, pDesc, riid, ppvHeap, AffinityMask);

    std::vector<ID3D12Heap*> Heaps;
    Heaps.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    D3D12_HEAP_PROPERTIES& Properties = (const_cast<D3D12_HEAP_DESC*>(pDesc))->Properties;
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12Heap* Heap = nullptr;
                Properties.CreationNodeMask = AffinityIndexToNodeMask(i) & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
                Properties.VisibleNodeMask = LDAAllNodeMasks() & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;

                HRESULT hr = Device->CreateHeap(pDesc, IID_PPV_ARGS(&Heap));
                if (S_OK == hr)
                {
                    Heaps[i] = Heap;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityHeap* Heap = new CD3DX12AffinityHeap(this, &(Heaps[0]), (UINT)Heaps.size());
    (*ppvHeap) = Heap;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreatePlacedResource(
    CD3DX12AffinityHeap* pHeap,
    UINT64 HeapOffset,
    const D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreatePlacedResource, pHeap, HeapOffset, pDesc, InitialState, pOptimizedClearValue, AffinityMask);

    std::vector<ID3D12Resource*> Resources;
    Resources.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
#if D3DXAFFINITY_TILE_MAPPING_GPUVA
        if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return E_NOTIMPL;
        }
#endif

        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12Resource* Resource = nullptr;

                HRESULT hr = Device->CreatePlacedResource(
                    pHeap->GetChildObject(i),
                    HeapOffset,
                    pDesc,
                    InitialState,
                    pOptimizedClearValue,
                    IID_PPV_ARGS(&Resource));
                if (S_OK == hr)
                {
                    Resources[i] = Resource;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityResource* Resource = new CD3DX12AffinityResource(this, &(Resources[0]), (UINT)Resources.size());
    (*ppvResource) = Resource;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateReservedResource(
    const D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES InitialState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riid,
    void** ppvResource,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateReservedResource, pDesc, InitialState, pOptimizedClearValue, AffinityMask);

    std::vector<ID3D12Resource*> Resources;
    Resources.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
#if D3DXAFFINITY_TILE_MAPPING_GPUVA
        if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return E_NOTIMPL;
        }
#endif
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12Resource* Resource = nullptr;

                HRESULT hr = Device->CreateReservedResource(
                    pDesc,
                    InitialState,
                    pOptimizedClearValue,
                    IID_PPV_ARGS(&Resource));
                if (S_OK == hr)
                {
                    Resources[i] = Resource;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityResource* Resource = new CD3DX12AffinityResource(this, &(Resources[0]), (UINT)Resources.size());
    (*ppvResource) = Resource;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::MakeResident(
    UINT NumObjects,
    CD3DX12AffinityPageable* const* ppObjects,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::MakeResident, NumObjects, ppObjects, AffinityMask);

    std::vector<ID3D12Pageable*> Objects(NumObjects);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                for (UINT j = 0; j < NumObjects; ++j)
                {
                    Objects[j] = static_cast<CD3DX12AffinityPageable*>(ppObjects[j])->mPageables[i];
                }
                HRESULT const hr = Device->MakeResident(NumObjects, Objects.data());
                if (S_OK != hr)
                {
                    return hr;
                }
            }
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::Evict(
    UINT NumObjects,
    CD3DX12AffinityPageable* const* ppObjects,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::Evict, NumObjects, ppObjects, AffinityMask);

    std::vector<CD3DX12AffinityPageable*> AffinityPageables;
    AffinityPageables.resize(NumObjects);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();

    for (UINT i = 0; i < NumObjects; ++i)
    {
        AffinityPageables[i] = static_cast<CD3DX12AffinityPageable*>(ppObjects[i]);
    }

    std::vector<ID3D12Pageable*> Pageables;
    Pageables.resize(NumObjects);
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                for (UINT j = 0; j < NumObjects; ++j)
                {
                    Pageables[j] = AffinityPageables[j]->mPageables[i];
                }

                HRESULT const hr = Device->Evict(NumObjects, Pageables.data());
                if (S_OK != hr)
                {
                    return hr;
                }
            }
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateFence(
    UINT64 InitialValue,
    D3D12_FENCE_FLAGS Flags,
    REFIID riid,
    void** ppFence,
    bool IsSingular)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateFence, InitialValue, Flags, IsSingular);

    std::vector<ID3D12Fence*> Fences;
    Fences.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (IsSingular) ? 1 : GetNodeMask();

    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        ID3D12Fence* Fence = nullptr;
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
			if (((1 << i) & EffectiveAffinityMask) != 0)
			{
                HRESULT const hr = Device->CreateFence(InitialValue, Flags, IID_PPV_ARGS(&Fence));
                if (S_OK == hr)
                {
                    Fences[i] = Fence;
                }
                else
                {
                    return hr;
                }
            }
        }
    }

    CD3DX12AffinityFence* Fence = new CD3DX12AffinityFence(this, &(Fences[0]), IsSingular?1:(UINT)Fences.size());
    (*ppFence) = Fence;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::GetDeviceRemovedReason(UINT AffinityIndex)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::GetDeviceRemovedReason, AffinityIndex);
    return mDevices[AffinityIndex]->GetDeviceRemovedReason();
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::GetCopyableFootprints(
    const D3D12_RESOURCE_DESC* pResourceDesc,
    UINT FirstSubresource,
    UINT NumSubresources,
    UINT64 BaseOffset,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
    UINT* pNumRows,
    UINT64* pRowSizeInBytes,
    UINT64* pTotalBytes,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::GetCopyableFootprints, pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pNumRows, pRowSizeInBytes, pTotalBytes, AffinityMask);

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        Device->GetCopyableFootprints(
            pResourceDesc,
            FirstSubresource,
            NumSubresources,
            BaseOffset,
            pLayouts,
            pNumRows,
            pRowSizeInBytes,
            pTotalBytes);
    }
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateQueryHeap(
    const D3D12_QUERY_HEAP_DESC* pDesc,
    REFIID riid,
    void** ppvHeap,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateQueryHeap, pDesc, AffinityMask);

    D3D12_QUERY_HEAP_DESC Desc = *pDesc;

    std::vector<ID3D12QueryHeap*> QueryHeaps;
    QueryHeaps.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12QueryHeap* QueryHeap = nullptr;

                Desc.NodeMask = AffinityIndexToNodeMask(i);
				Desc.NodeMask &= CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;

                HRESULT const hr = Device->CreateQueryHeap(&Desc, IID_PPV_ARGS(&QueryHeap));
                if (S_OK == hr)
                {
                    QueryHeaps[i] = QueryHeap;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityQueryHeap* QueryHeap = new CD3DX12AffinityQueryHeap(this, &(QueryHeaps[0]), (UINT)QueryHeaps.size());
    (*ppvHeap) = QueryHeap;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::SetStablePowerState(
    BOOL Enable,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::SetStablePowerState, Enable, AffinityMask);

    for (UINT ActiveIndex = 0; ActiveIndex < mNumActiveDevices; ++ActiveIndex)
    {
        ID3D12Device* Device = mDevices[ActiveIndex];
        UINT const i = ActiveIndex;

        HRESULT const hr = Device->SetStablePowerState(Enable);

        if (S_OK != hr)
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityDevice::CreateCommandSignature(
    const D3D12_COMMAND_SIGNATURE_DESC* pDesc,
    CD3DX12AffinityRootSignature* pRootSignature,
    REFIID riid,
    void** ppvCommandSignature,
    UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::CreateCommandSignature, pDesc, pRootSignature, AffinityMask);

    D3D12_COMMAND_SIGNATURE_DESC Desc = *pDesc;
    CD3DX12AffinityRootSignature* RootSignature = static_cast<CD3DX12AffinityRootSignature*>(pRootSignature);

    std::vector<ID3D12CommandSignature*> CommandSignatures;
    CommandSignatures.resize(D3DXAFFINITY_GET_NODE_COUNT, nullptr);
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();

    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        ID3D12Device* Device = mDevices[0];
        for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
        {
            if (((1 << i) & EffectiveAffinityMask) != 0)
            {
                ID3D12CommandSignature* CommandSignature = nullptr;

                Desc.NodeMask = AffinityIndexToNodeMask(i);
				Desc.NodeMask &= CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
                HRESULT const hr = Device->CreateCommandSignature(&Desc, RootSignature ? RootSignature->mRootSignatures[i] : nullptr, IID_PPV_ARGS(&CommandSignature));
                if (S_OK == hr)
                {
                    CommandSignatures[i] = CommandSignature;
                }
                else
                {
                    return hr;
                }
            }
        }
    }
    CD3DX12AffinityCommandSignature* CommandSignature = new CD3DX12AffinityCommandSignature(this, &(CommandSignatures[0]), (UINT)CommandSignatures.size());
    (*ppvCommandSignature) = CommandSignature;

    return S_OK;
}

void STDMETHODCALLTYPE CD3DX12AffinityDevice::GetResourceTiling(
    CD3DX12AffinityResource* pTiledResource,
    UINT* pNumTilesForEntireResource,
    D3D12_PACKED_MIP_INFO* pPackedMipDesc,
    D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
    UINT* pNumSubresourceTilings,
    UINT FirstSubresourceTilingToGet,
    D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips,
    UINT AffinityIndex)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::GetResourceTiling, pTiledResource, pNumTilesForEntireResource, pPackedMipDesc, pStandardTileShapeForNonPackedMips, AffinityIndex);

    mDevices[AffinityIndex]->GetResourceTiling(
        pTiledResource->mResources[AffinityIndex],
        pNumTilesForEntireResource,
        pPackedMipDesc,
        pStandardTileShapeForNonPackedMips,
        pNumSubresourceTilings,
        FirstSubresourceTilingToGet,
        pSubresourceTilingsForNonPackedMips);
}

LUID STDMETHODCALLTYPE CD3DX12AffinityDevice::GetAdapterLuid(UINT AffinityIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetAdapterLuid, AffinityIndex);

    return mDevices[AffinityIndex]->GetAdapterLuid();
}

ID3D12Device* CD3DX12AffinityDevice::GetChildObject(UINT AffinityIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetChildObject, AffinityIndex);

    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        return mDevices[0];
    }
    else
    {
        return mDevices[AffinityIndex];
    }
}

bool CD3DX12AffinityDevice::IsD3D()
{
    return true;
}

CD3DX12AffinityDevice::CD3DX12AffinityDevice(ID3D12Device** devices, UINT Count, EAffinityMode::Mask affinitymode)
    : CD3DX12AffinityObject(this, reinterpret_cast<IUnknown**>(devices), Count)
{
	D3DX12_AFFINITY_CALL0(ID3D12Device::CD3DX12AffinityDevice);

	mInternal = new Internal_CD3DX12AffinityDevice();

#ifdef D3DXAFFINITY_DEBUG_OBJECT_NAME
    mObjectTypeName = L"Device";
#endif
    mAffinityMode = affinitymode;
    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES; i++)
    {
        if (i < Count)
        {
            mDevices[i] = devices[i];
        }
        else
        {
            mDevices[i] = nullptr;
        }
    }
    if (mAffinityMode == EAffinityMode::LDA)
    {
        mLDANodeCount = min(mDevices[0]->GetNodeCount(), D3DX12AFFINITY_MAX_ACTIVE_NODES);
		mLDANodeCount = max(mLDANodeCount, D3DX12AFFINITY_EMULATE_MIN_NODES);
    }
    else
    {
        mLDANodeCount = 0;
    }
    mDeviceCount = Count;
    if (mAffinityMode == EAffinityMode::LDA)
    {
		CD3DX12AffinityDeviceStatics::g_CachedNodeCount = mLDANodeCount;
		CD3DX12AffinityDeviceStatics::g_CachedNodeMask = (1 << mLDANodeCount) - 1;
		CD3DX12AffinityDeviceStatics::g_CachedActualNodeCount = mDevices[0]->GetNodeCount();
		CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask = (1 << CD3DX12AffinityDeviceStatics::g_CachedActualNodeCount) - 1;
    }
    else
    {
		CD3DX12AffinityDeviceStatics::g_CachedNodeCount = mDeviceCount;
		CD3DX12AffinityDeviceStatics::g_CachedNodeMask = (1 << mDeviceCount) - 1;
		CD3DX12AffinityDeviceStatics::g_CachedActualNodeCount = mDeviceCount;
		CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask = (1 << CD3DX12AffinityDeviceStatics::g_CachedActualNodeCount) - 1;
    }
    mAffinityRenderingMode = EAffinityRenderingMode::AFR;

    mParentDevice = this;
    // Re-calculate the affinity based on the effective affinity node mask.
    SetAffinity(GetNodeMask());

    UpdateActiveDevices();

    // Initialize sync queues for cross frame resources.
    if (GetAffinityMode() == EAffinityMode::LDA)
    {
        for (UINT i = 0; i < D3DXAFFINITY_GET_NODE_COUNT; i++)
        {
            D3D12_COMMAND_QUEUE_DESC desc;
            desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            desc.Priority = 0;
            desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            desc.NodeMask = 1 << i;
			desc.NodeMask &= CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;

            mDevices[0]->CreateCommandQueue(&desc, IID_PPV_ARGS(&mSyncCommandQueues[i]));
            mDevices[0]->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mSyncFences[i]));
        }
    }
}

CD3DX12AffinityDevice::~CD3DX12AffinityDevice()
{
	D3DX12_AFFINITY_CALL0(ID3D12Device::~CD3DX12AffinityDevice);
    for (UINT i = 0; i < D3DXAFFINITY_GET_NODE_COUNT; i++)
    {
        mSyncCommandQueues[i]->Release();
        mSyncFences[i]->Release();
    }

	delete mInternal;
}

UINT CD3DX12AffinityDevice::GetDeviceCount()
{
    return mDeviceCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE CD3DX12AffinityDevice::GetCPUHeapPointer(D3D12_CPU_DESCRIPTOR_HANDLE const& Original, UINT const NodeIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetCPUHeapPointer, Original, NodeIndex);

    if (D3DXAFFINITY_GET_NODE_COUNT == 1 || Original.ptr == 0)
    {
        return Original;
    }
#ifdef D3DX_AFFINITY_ENABLE_HEAP_POINTER_VALIDATION
    // Validation
    {
        std::lock_guard<std::mutex> lock(MutexPointerRanges);

        bool IsOK = false;
        for (auto Range : GetParentDevice()->CPUHeapPointerRanges)
        {
            if (Original.ptr >= Range.first && Original.ptr < Range.second)
            {
                IsOK = true;
                break;
            }
        }

        if (!IsOK)
        {
            D3DX12AFFINITY_LOG("Found a pointer outside of expected memory ranges!\n");
            DEBUG_FAIL_MESSAGE(L"Found a pointer outside of expected memory ranges!\n");
        }

    }
#endif
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = static_cast<size_t>(((UINT64*)Original.ptr)[NodeIndex]);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE CD3DX12AffinityDevice::GetGPUHeapPointer(D3D12_GPU_DESCRIPTOR_HANDLE const& Original, UINT const NodeIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetGPUHeapPointer, Original, NodeIndex);
    if (D3DXAFFINITY_GET_NODE_COUNT == 1)
    {
        return Original;
    }
#ifdef D3DX_AFFINITY_ENABLE_HEAP_POINTER_VALIDATION
    // Validation
    {
        std::lock_guard<std::mutex> lock(MutexPointerRanges);

        bool IsOK = false;
        for (auto Range : GetParentDevice()->GPUHeapPointerRanges)
        {
            if (Original.ptr >= Range.first && Original.ptr < Range.second)
            {
                IsOK = true;
                break;
            }
        }

        if (!IsOK)
        {
            D3DX12AFFINITY_LOG("Found a pointer outside of expected memory ranges!\n");
            DEBUG_FAIL_MESSAGE("Found a pointer outside of expected memory ranges!\n");
        }

    }
#endif
    D3D12_GPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = ((UINT64*)Original.ptr)[NodeIndex];
    return handle;
}

D3D12_GPU_VIRTUAL_ADDRESS CD3DX12AffinityDevice::GetGPUVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS const& Original, UINT const NodeIndex)
{
	//D3DX12_AFFINITY_CALL(ID3D12Device::GetGPUVirtualAddress, Original, NodeIndex);

    if (NodeIndex == 0)
        return Original;

    if (Original == 0)
        return 0;

#if D3DXAFFINITY_TILE_MAPPING_GPUVA
    if (GetAffinityMode() == EAffinityMode::LDA)
        return Original;
#endif


    // This function searches through our list of known GPU virtual addresses and finds the next lowest (or equal) address to the Original
    // The Original pointer is then assumed to be an offset into some segment of memory that starts at the next lowest address.
    // We use that offset against the equivalent base addresses across all devices.

    // TODO: It is inefficient to look through this every time i.e. for each DeviceIndex.
    // Possibly just cache the NextLowest here or change the interface to return a vector of offsetted pointers.
    // Should probably do both, since surely the user will be using heap pointers often and it would be nice
    // to simply save the base pointers for future lookup.
    // Of course this would all go away if we had a way to have consistent virtual addresses across all devices!
    // Also it might be possible to use proxy CPU memory allocated by the affinity layer to make this a simple lookup - see GetGPUHeapPointer.
    D3D12_GPU_VIRTUAL_ADDRESS NextLowest;
    D3D12_GPU_VIRTUAL_ADDRESS Return;

    {
        std::lock_guard<std::mutex> lock(MutexGPUVirtualAddresses);

        auto NextGreatestIterator = GPUVirtualAddresses.upper_bound(Original);
        auto NextLowestIterator = --NextGreatestIterator;
        NextLowest = NextLowestIterator->first;
        Return = NextLowestIterator->second[NodeIndex];
    }

    D3D12_GPU_VIRTUAL_ADDRESS const Offset = Original - NextLowest;
    Return += Offset;

    return Return;
}

void CD3DX12AffinityDevice::WriteApplicationMessage(D3D12_MESSAGE_SEVERITY const Severity, char const* const Message)
{
    D3DX12AFFINITY_LOG("Writing application message: %s\n", Message);
    if (InfoQueue)
    {
        InfoQueue->AddApplicationMessage(Severity, Message);
    }
    else
    {
        D3DX12AFFINITY_LOG("No InfoQueue to write application message.\n");
    }
}

UINT CD3DX12AffinityDevice::AffinityIndexToNodeMask(UINT const Index)
{
#ifndef D3DX12_SIMULATE_LDA_ON_SINGLE_NODE  
    if (mAffinityMode == EAffinityMode::LDA)
        return ( 1 << Index) & CD3DX12AffinityDeviceStatics::g_CachedActualNodeMask;
    else
#endif
        return 0;
}

EAffinityMode::Mask CD3DX12AffinityDevice::GetAffinityMode()
{
	//D3DX12_AFFINITY_CALL0(ID3D12Device::GetAffinityMode);
    return mAffinityMode;
}

EAffinityRenderingMode::Mask CD3DX12AffinityDevice::GetAffinityRenderingMode()
{
	//D3DX12_AFFINITY_CALL0(ID3D12Device::GetAffinityRenderingMode);
    return mAffinityRenderingMode;
}

void CD3DX12AffinityDevice::SetAffinityRenderingMode(EAffinityRenderingMode::Mask renderingmode)
{
	D3DX12_AFFINITY_CALL(ID3D12Device::SetAffinityRenderingMode, renderingmode);
    mAffinityRenderingMode = renderingmode;
}

UINT CD3DX12AffinityDevice::GetActiveNodeMask()
{
	//D3DX12_AFFINITY_CALL0(ID3D12Device::GetActiveNodeMask);
	return AffinityIndexToNodeMask(CD3DX12AffinityDeviceStatics::GetActiveNodeID());
}

void CD3DX12AffinityDevice::SwitchToNextNode()
{
	D3DX12_AFFINITY_CALL0(ID3D12Device::SwitchToNextNode);

#ifdef D3DXAFFINITY_SYNC_CROSS_FRAME_RESOURCES
    // Sync all cross frame resources.
    {
        std::lock_guard<std::mutex> lock(MutexSyncResources);
        for (UINT i = 0; i < mSyncResources.size(); i++)
        {
            mSyncResources[i]->BroadcastResourceForCrossGPUSync();
        }

        //#ifdef D3DX12AFFINITY_SERIALIZE_COMMANDLIST_EXECUTION
        ID3D12Fence* pFence;
        GetParentDevice()->mDevices[0]->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
        mSyncCommandQueues[mActiveNodeIndex]->Signal(pFence, 1);
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        pFence->SetEventOnCompletion(1, hEvent);
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
        pFence->Release();
        if (FAILED(mDevices[0]->GetDeviceRemovedReason()))
        {
            __debugbreak();
        }
        //#endif
    }
#endif

    CD3DX12AffinityDeviceStatics::SetActiveNodeID((CD3DX12AffinityDeviceStatics::GetActiveNodeID() + 1) % D3DXAFFINITY_GET_NODE_COUNT);
#ifdef D3DX12AFFINITY_ENABLE_SWITCHNODE_DEBUG
	D3DX12AFFINITY_LOG("CD3DX12AffinityGraphicsCommandList::SwitchToNextNode (active node update) %d %llX", CD3DX12AffinityDeviceStatics::GetActiveNodeID_RAW(), (unsigned long long)this);
#endif
}

