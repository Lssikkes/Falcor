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

#pragma warning(push, 0)


#include "d3dxaffinity/d3dx12affinity.h"
#include "d3dxaffinity/CD3DX12Utils.h"
#include "d3dxaffinity/CD3DX12Settings.h"

#include <locale>
#include <codecvt>
#include <string>
#include <comdef.h>

D3DX12AF_API void WriteHRESULTError(HRESULT const hr)
{
    _com_error err(hr, nullptr);
    D3DX12AFFINITY_LOG("HRESULT Failure: %d 0x%08X %s\n", hr, hr, err.ErrorMessage());
}

#pragma warning(pop)