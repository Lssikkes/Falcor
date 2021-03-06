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

#pragma once

#include "CD3DX12Utils.h"
#include "CD3DX12AffinityPageable.h"

class D3DX12AF_API __declspec(uuid("BE1D71C8-88FD-4623-ABFA-D0E546D12FAF")) CD3DX12AffinityFence : public CD3DX12AffinityPageable
{
public:
    UINT64 STDMETHODCALLTYPE GetCompletedValue(void);

    UINT64 STDMETHODCALLTYPE GetCompletedValue(UINT AffinityMask);

    // It is not recommended to use this function in multi-GPU scenarios 
    // as it operates only on the active GPU fence.
    // Please refactor your engine to use WaitOnFenceCompletion instead.
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(
        UINT64 Value,
        HANDLE hEvent);

    // The mask should resolve to a single Node Index.
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(
        UINT64 Value,
        HANDLE hEvent,
        UINT AffinityMask);

    HRESULT STDMETHODCALLTYPE WaitOnFenceCompletion(
        UINT64 Value);

    HRESULT STDMETHODCALLTYPE Signal(
        UINT64 Value);

    HRESULT STDMETHODCALLTYPE Signal(
        UINT64 Value,
        UINT AffinityIndex);

    CD3DX12AffinityFence(CD3DX12AffinityDevice* device, ID3D12Fence** fences, UINT Count);
    ~CD3DX12AffinityFence();
    ID3D12Fence* GetChildObject(UINT AffinityIndex);

	
    ID3D12Fence* mFences[D3DX12_MAX_ACTIVE_NODES];
	UINT mNumFences;
    HANDLE mCompletionEvent;

private:
    CD3DX12AffinityDevice* mDevice;
};
