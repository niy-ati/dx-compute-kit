# dx-compute-kit — BACKEND_MAP (short)

Where this sits relative to existing dcompute backends:

| Backend | Device IR / bitcode | Host launch |
|---------|---------------------|-------------|
| CUDA | PTX | CUDA runtime |
| OpenCL | SPIR-V (ocl) | OpenCL |
| Vulkan (GSoC track) | SPIR-V (vulkan) | Vulkan |
| Metal (GSoC track) | Metal IR / metallib | Metal ObjC |
| **DirectX (this kit)** | **DXIL** | **D3D12** |

LDC metadata for DirectX compute (what fixtures check):

- triple: `dxil-pc-shadermodelN.M-compute`
- `"hlsl.shader"="compute"`
- `"hlsl.numthreads"="X,Y,Z"`
- optional before prepare: `"exp-shader"="cs"`

Wrapper note (same idea as Vulkan): D `@kernel` body can stay a “core”
function; a zero-arg `*_kernel` entry holds the HLSL attrs and unpacks an
arg buffer. The saxpy host here uses a normal DXC root signature (UAV + CBV),
i.e. the **proven host path**. LDC’s provisional arg-buffer ABI is still being
debugged against D3D12 PSO creation — do not confuse the two.
