///*********************************************************
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

// [[ ORIGINAL CONFIG ]]

// Max nodes as defined in source file
#define D3DX12AFFINITY_MAX_ACTIVE_NODES_CAP 2

// Emulate number of nodes if machine has less than N nodes available
#define D3DX12AFFINITY_EMULATE_MIN_NODES 2

// Simulate LDA mode on a single node
//#define D3DX12_SIMULATE_LDA_ON_SINGLE_NODE

// Set clear color to white for each clear
//#define D3DX12AFFINITY_DEBUG_CLEAR_WHITE

// Insert a fence, wait and a check for device removed after every commandlist
// useful for debugging the source command-list when a TDR occurs.
//#define D3DX12AFFINITY_SERIALIZE_COMMANDLIST_EXECUTION


//#define D3DXAFFINITY_SYNC_CROSS_FRAME_RESOURCES

// On LDA devices, makes all buffers have a single GPUVA by either having a single
// resource in system memory, or having a single reserved buffer mapped to separate
// heaps on each GPU (via unicast page table mappings). Removes any GPUVA translation
// overhead.
//#define D3DXAFFINITY_TILE_MAPPING_GPUVA 1

// Just a fun experiment to see what happens when all buffers are accessed from remote GPUs.
//#define D3DXAFFINITY_FORCE_REMOTE_TILE_MAPPING_GPUVA 1

#define D3DXAFFINITY_ALWAYS_RESET_ALL_COMMAND_LISTS 1

////////////////////////////
// DEBUG CONFIG ////////////
////////////////////////////

// Logging options. (e.g. memory mapped, every ECL)
//#define ENABLE_DEBUG_LOG 1
//#define ENABLE_RELEASE_LOG 1
//#define D3DX_AFFINITY_ENABLE_HEAP_POINTER_VALIDATION

// Instead of using GetWriteWatch, just upload ALL mapped data when unmapped or ECL.
// This is mostly just used to prove how SLOW it is not to use GetWriteWatch.
//#define DO_FULL_MAPPED_MEM_COPY


// [[ EXTRA CONFIG ]]

// Extra error checking
//#define D3DX12AFFINITY_EC_VALIDATE_ACTIVE_ID
//#define D3DX12AFFINITY_EC_VALIDATE_NULL_BARRIERS
//#define D3DX12AFFINITY_EC_VALIDATE_DESCRIPTORS


// Enable logging.
//#define D3DX12AFFINITY_ENABLE_LOGGING

// Enable high intensity debug logging flags
//#define D3DX12AFFINITY_ENABLE_RTV_DEBUG					/* (extra information about rtvs, usage/creation/destruction) */
//#define D3DX12AFFINITY_ENABLE_SWITCHNODE_DEBUG			/* (extra information about the next node switch) */
//#define D3DX12AFFINITY_ENABLE_GFXCMDLIST_DEBUG			/* (extra information about calls in graphics command list) *
//#define D3DX12AFFINITY_ENABLE_DEVICE_DEBUG				/* (extra information about calls in device) */
//#define D3DX12AFFINITY_ENABLE_CMDQUEUE_DEBUG				/* (extra information about calls in command queues) */
//#define D3DX12AFFINITY_ENABLE_FENCE_DEBUG					/* (extra information about calls in fences) */

// [[ PREPROCESSOR IMPLEMENTATIONS ]]
#if defined(D3DX12AFFINITY_ENABLE_LOGGING)
	#define D3DX12AFFINITY_LOG(fmt, ...) { char __buffer[1024]; sprintf(__buffer, ("D3DXAffinity: " fmt), __VA_ARGS__); if(__buffer[strlen(__buffer)-1] != '\n') { strcat(__buffer, "\n"); } OutputDebugStringA(__buffer);  }
#else
	#define D3DX12AFFINITY_LOG(fmt, ...) {}
#endif

#define D3DX12AFFINITY_MAX_ACTIVE_NODES (int)(((D3DX12AffinityGetNodeLimit()<D3DX12AFFINITY_MAX_ACTIVE_NODES_CAP)?D3DX12AffinityGetNodeLimit():D3DX12AFFINITY_MAX_ACTIVE_NODES_CAP))
#define D3DX12AFFINITY_BREAK __debugbreak()

#if D3DX12AFFINITY_MAX_ACTIVE_NODES_CAP > D3DX12_MAX_ACTIVE_NODES
	#error "Source max active nodes cannot exceed D3DX12_MAX_ACTIVE_NODES in CD3DX12Utils.h"
#endif
