#ifndef D3DX12_AFFINITY_REAL_TYPEDEFS
#define D3DX12_AFFINITY_REAL_TYPEDEFS

typedef	ID3D12Object 									R_ID3D12Object;
typedef	ID3D12DeviceChild 								R_ID3D12DeviceChild;
typedef	ID3D12Pageable 									R_ID3D12Pageable;
typedef	ID3D12RootSignature 							R_ID3D12RootSignature;
typedef	ID3D12RootSignatureDeserializer					R_ID3D12RootSignatureDeserializer;
typedef	ID3D12Heap 										R_ID3D12Heap;
typedef	ID3D12Resource 									R_ID3D12Resource;
typedef	ID3D12CommandAllocator 							R_ID3D12CommandAllocator;
typedef	ID3D12Fence 									R_ID3D12Fence;
typedef	ID3D12PipelineState 							R_ID3D12PipelineState;
typedef	ID3D12DescriptorHeap 							R_ID3D12DescriptorHeap;
typedef	ID3D12QueryHeap									R_ID3D12QueryHeap;
typedef	ID3D12CommandSignature 							R_ID3D12CommandSignature;
typedef	ID3D12CommandList 								R_ID3D12CommandList;
typedef	ID3D12GraphicsCommandList 						R_ID3D12GraphicsCommandList;
typedef	ID3D12CommandQueue 								R_ID3D12CommandQueue;
typedef	ID3D12Device									R_ID3D12Device;
typedef	IDXGISwapChain									R_IDXGISwapChain;
typedef	D3D12_GRAPHICS_PIPELINE_STATE_DESC				R_D3D12_GRAPHICS_PIPELINE_STATE_DESC;
typedef	D3D12_COMPUTE_PIPELINE_STATE_DESC				R_D3D12_COMPUTE_PIPELINE_STATE_DESC;
typedef	D3D12_RESOURCE_TRANSITION_BARRIER				R_D3D12_RESOURCE_TRANSITION_BARRIER;
typedef	D3D12_RESOURCE_ALIASING_BARRIER					R_D3D12_RESOURCE_ALIASING_BARRIER;
typedef	D3D12_RESOURCE_UAV_BARRIER						R_D3D12_RESOURCE_UAV_BARRIER;
typedef	D3D12_RESOURCE_BARRIER							R_D3D12_RESOURCE_BARRIER;
typedef	D3D12_TEXTURE_COPY_LOCATION						R_D3D12_TEXTURE_COPY_LOCATION;
typedef	CD3DX12_TEXTURE_COPY_LOCATION					R_CD3DX12_TEXTURE_COPY_LOCATION;
typedef	CD3DX12_RESOURCE_DESC							R_CD3DX12_RESOURCE_DESC;
typedef	CD3DX12_RESOURCE_BARRIER						R_CD3DX12_RESOURCE_BARRIER;
#endif

#define ID3D12Object 									CD3DX12AffinityObject						
#define ID3D12DeviceChild 								CD3DX12AffinityDeviceChild					
#define ID3D12Pageable 									CD3DX12AffinityPageable						
#define ID3D12RootSignature 							CD3DX12AffinityRootSignature				
#define ID3D12RootSignatureDeserializer					CD3DX12AffinityRootSignatureDeserializer	
#define ID3D12Heap 										CD3DX12AffinityHeap							
#define ID3D12Resource 									CD3DX12AffinityResource						
#define ID3D12CommandAllocator 							CD3DX12AffinityCommandAllocator				
#define ID3D12Fence 									CD3DX12AffinityFence						
#define ID3D12PipelineState 							CD3DX12AffinityPipelineState				
#define ID3D12DescriptorHeap 							CD3DX12AffinityDescriptorHeap				
#define ID3D12QueryHeap									CD3DX12AffinityQueryHeap					
#define ID3D12CommandSignature 							CD3DX12AffinityCommandSignature				
#define ID3D12CommandList 								CD3DX12AffinityCommandList					
#define ID3D12GraphicsCommandList 						CD3DX12AffinityGraphicsCommandList			
#define ID3D12CommandQueue 								CD3DX12AffinityCommandQueue					
#define ID3D12Device									CD3DX12AffinityDevice										
#define IDXGISwapChain									CDXGIAffinitySwapChain
#define D3D12_GRAPHICS_PIPELINE_STATE_DESC				D3DX12_AFFINITY_GRAPHICS_PIPELINE_STATE_DESC
#define D3D12_COMPUTE_PIPELINE_STATE_DESC				D3DX12_AFFINITY_COMPUTE_PIPELINE_STATE_DESC	
#define D3D12_RESOURCE_TRANSITION_BARRIER				D3DX12_AFFINITY_RESOURCE_TRANSITION_BARRIER	
#define D3D12_RESOURCE_ALIASING_BARRIER					D3DX12_AFFINITY_RESOURCE_ALIASING_BARRIER	
#define D3D12_RESOURCE_UAV_BARRIER						D3DX12_AFFINITY_RESOURCE_UAV_BARRIER		
#define D3D12_RESOURCE_BARRIER							D3DX12_AFFINITY_RESOURCE_BARRIER			
#define D3D12_TEXTURE_COPY_LOCATION						D3DX12_AFFINITY_TEXTURE_COPY_LOCATION		
#define CD3DX12_TEXTURE_COPY_LOCATION					CD3DX12_AFFINITY_TEXTURE_COPY_LOCATION	
#define CD3DX12_RESOURCE_DESC							CD3DX12_AFFINITY_RESOURCE_DESC				
#define CD3DX12_RESOURCE_BARRIER						CD3DX12_AFFINITY_RESOURCE_BARRIER

				