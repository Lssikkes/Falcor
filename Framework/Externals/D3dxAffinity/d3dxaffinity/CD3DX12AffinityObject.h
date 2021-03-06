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

class D3DX12AF_API __declspec(uuid("BE1D71C8-88FD-4623-ABFA-D0E546D12FAF")) CD3DX12AffinityObject : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		__RPC__deref_out void __RPC_FAR* __RPC_FAR* ppvObject
	);

    HRESULT STDMETHODCALLTYPE GetPrivateData(
        _In_  REFGUID guid,
        _Inout_  UINT* pDataSize,
        _Out_writes_bytes_opt_(*pDataSize)  void* pData,
		UINT AffinityMask = 0xFFFFFFFF);

    HRESULT STDMETHODCALLTYPE SetPrivateData(
        _In_  REFGUID guid,
        _In_  UINT DataSize,
        _In_reads_bytes_opt_(DataSize)  const void* pData,
        UINT AffinityMask = 0xFFFFFFFF);

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        _In_  REFGUID guid,
        _In_opt_  const IUnknown* pData,
		UINT AffinityMask = 0xFFFFFFFF);

    HRESULT STDMETHODCALLTYPE SetName(
        _In_z_  LPCWSTR Name,
		UINT AffinityMask = 0xFFFFFFFF);

    virtual void STDMETHODCALLTYPE SetAffinity(
        _In_  UINT AffinityMask);

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    CD3DX12AffinityObject(CD3DX12AffinityDevice* device, IUnknown** objects, UINT Count);
    virtual ~CD3DX12AffinityObject();

    CD3DX12AffinityDevice* GetParentDevice();

    IUnknown* mObjects[D3DX12_MAX_ACTIVE_NODES];
    UINT mAffinityMask = 0;
    UINT mReferenceCount;

#ifdef D3DXAFFINITY_DEBUG_OBJECT_NAME
    std::wstring mObjectTypeName;
    std::wstring mObjectDebugName;
#endif
    CD3DX12AffinityDevice* mParentDevice;

    static UINT AffinityIndexToMask(UINT const Index);
    UINT GetNodeMask();

    UINT GetNodeCount();
    UINT GetActiveNodeIndex();
	UINT GetAffinityMask() { return mAffinityMask; }

protected:
    virtual bool IsD3D() = 0;

private:
    // Non-copyable
    CD3DX12AffinityObject(CD3DX12AffinityObject const&);
    CD3DX12AffinityObject& operator=(CD3DX12AffinityObject const&);
};


