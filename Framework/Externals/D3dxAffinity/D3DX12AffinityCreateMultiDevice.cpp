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

/**
 * This is the standard (only) method for creating a D3DX12 Affinity device. It takes
 * an array of ID3D12Devices and does some very minimal setup.
 */

#include "d3dxaffinity/d3dx12affinity.h"
#include "d3dxaffinity/CD3DX12Utils.h"
#include "d3dxaffinity/CD3DX12Settings.h"

static int g_d3dx12affinity_nodeLimit = D3DX12AFFINITY_MAX_ACTIVE_NODES_CAP;

D3DX12AF_API unsigned int WINAPI D3DX12AffinityGetNodeLimit()
{
	return g_d3dx12affinity_nodeLimit;
}

D3DX12AF_API void WINAPI D3DX12AffinitySetNodeLimit(unsigned int nodeLimit)
{
	g_d3dx12affinity_nodeLimit = nodeLimit;
}

D3DX12AF_API HRESULT WINAPI D3DX12AffinityCreateLDADevice(
    ID3D12Device* Device,
    CD3DX12AffinityDevice** ppDevice)
{
	CD3DX12AffinityDeviceStatics::SetActiveNodeID(0);

    CD3DX12AffinityDevice* Instance = new CD3DX12AffinityDevice(&Device, 1, EAffinityMode::LDA);
    Device->AddRef();

#ifdef DEBUG
    ID3D12Debug* debugController = nullptr;
    if (!CD3DX12AffinityDevice::InfoQueue && SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        ID3D12InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            CD3DX12AffinityDevice::InfoQueue = infoQueue;
        }
    }
#endif

    (*ppDevice) = Instance;
    return S_OK;
}
