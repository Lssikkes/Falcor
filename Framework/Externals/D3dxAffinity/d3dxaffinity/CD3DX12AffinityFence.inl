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

#ifndef D3DX12AFFINITY_ENABLE_FENCE_DEBUG
#define D3DX12_AFFINITY_CALL0(x)
#define D3DX12_AFFINITY_CALL(x, ...)
#else
#include "d3dx12affinity_debug.h"
#define D3DX12_AFFINITY_CALL0(x) D3DX12_AFFINITY_DOCALL0(x)
#define D3DX12_AFFINITY_CALL(x, ...) D3DX12_AFFINITY_DOCALL(x, __VA_ARGS__)
#endif

UINT64 STDMETHODCALLTYPE CD3DX12AffinityFence::GetCompletedValue(void)
{
    UINT64 Minimum = mFences[0]->GetCompletedValue();
    for (UINT i = 1; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
    {
        if (mFences[i] && ((1 << i) & mAffinityMask) != 0)
        {

            UINT64 CompletedValue = mFences[i]->GetCompletedValue();
            Minimum = min(CompletedValue, Minimum);
        }
    }

	D3DX12_AFFINITY_CALL(ID3D12Fence::GetCompletedValue, mAffinityMask, Minimum);
    return Minimum;
}

UINT64 CD3DX12AffinityFence::GetCompletedValue(UINT AffinityMask)
{
    UINT64 Minimum = 0 - 1ull;
    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();

    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
    {
        if (mFences[i] && ((1 << i) & EffectiveAffinityMask) != 0)
        {
            UINT64 CompletedValue = mFences[i]->GetCompletedValue();
            Minimum = min(CompletedValue, Minimum);
        }
    }

	D3DX12_AFFINITY_CALL(ID3D12Fence::GetCompletedValue, Minimum, AffinityMask);
    return Minimum;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityFence::SetEventOnCompletion(
    UINT64 Value,
    HANDLE hEvent)
{
	D3DX12_AFFINITY_CALL(ID3D12Fence::SetEventOnCompletion, Value, hEvent);

    UINT i = GetActiveNodeIndex();
    ID3D12Fence* Fence = mFences[i];
    return Fence->SetEventOnCompletion(Value, hEvent);
}

HRESULT CD3DX12AffinityFence::SetEventOnCompletion(UINT64 Value, HANDLE hEvent, UINT AffinityMask)
{
	D3DX12_AFFINITY_CALL(ID3D12Fence::SetEventOnCompletion, Value, hEvent, AffinityMask);

    UINT EffectiveAffinityMask = (AffinityMask == 0) ? GetNodeMask() : AffinityMask & GetNodeMask();
    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
    {
        if (mFences[i] && ((1 << i) & EffectiveAffinityMask) != 0)
        {
            return mFences[i]->SetEventOnCompletion(Value, hEvent);
        }
    }
    //should never get here, mask is incorrect
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityFence::WaitOnFenceCompletion(
    UINT64 Value)
{
	D3DX12_AFFINITY_CALL(ID3D12Fence::WaitOnFenceCompletion, Value);

    std::vector<HANDLE> Events;

    UINT EventCount = 0;
    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
    {
        if (mFences[i] && ((1 << i) & mAffinityMask) != 0)
        {
            ID3D12Fence* Fence = mFences[i];
			if (Fence == nullptr)
				continue;
			if (Fence->GetCompletedValue() == Value)
				continue;

            Events.push_back(0);
            Events[EventCount] = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
            HRESULT const hr = Fence->SetEventOnCompletion(Value, Events[EventCount]);

            if (hr != S_OK)
            {
                return hr;
            }

            ++EventCount;
        }
    }

	if (EventCount > 0)
	{
		DWORD res = WaitForMultipleObjects((DWORD)EventCount, Events.data(), TRUE, INFINITE);
		

		// BUGFIX: Close events
		for (auto& event : Events)
			CloseHandle(event);
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CD3DX12AffinityFence::Signal(
    UINT64 Value)
{
	D3DX12_AFFINITY_CALL(ID3D12Fence::Signal, Value);

    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES;i++)
    {
        if (((1 << i) & mAffinityMask) != 0)
        {
            ID3D12Fence* Fence = mFences[i];

            HRESULT const hr = Fence->Signal(Value);

            if (hr != S_OK)
            {
                return hr;
            }
        }
    }

    return S_OK;
}

HRESULT CD3DX12AffinityFence::Signal(UINT64 Value, UINT AffinityIndex)
{
	D3DX12_AFFINITY_CALL(ID3D12Fence::Signal, Value, AffinityIndex);

    return mFences[AffinityIndex]->Signal(Value);
}

CD3DX12AffinityFence::CD3DX12AffinityFence(CD3DX12AffinityDevice* device, ID3D12Fence** fences, UINT Count)
    : CD3DX12AffinityPageable(device, reinterpret_cast<ID3D12Pageable**>(fences), Count)
{
	D3DX12_AFFINITY_CALL0(ID3D12Fence::ID3D12Fence);

	mNumFences = Count;

    for (UINT i = 0; i < D3DX12AFFINITY_MAX_ACTIVE_NODES; i++)
    {
        if (i < Count)
        {
            mFences[i] = fences[i];
        }
        else
        {
            mFences[i] = nullptr;
        }
    }
#ifdef D3DXAFFINITY_DEBUG_OBJECT_NAME
    mObjectTypeName = L"Fence";
#endif
    mDevice = device;
}

CD3DX12AffinityFence::~CD3DX12AffinityFence()
{
}

ID3D12Fence* CD3DX12AffinityFence::GetChildObject(UINT AffinityIndex)
{
    return mFences[AffinityIndex];
}
