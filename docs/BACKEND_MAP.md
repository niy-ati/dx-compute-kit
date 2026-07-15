# Backend map

How DirectX fits relative to existing dcompute targets:

| Backend | Device IR | Host |
|---------|-----------|------|
| CUDA | PTX | CUDA runtime |
| OpenCL | SPIR-V | OpenCL |
| Vulkan | SPIR-V | Vulkan |
| Metal | Metal IR | Metal |
| DirectX (this kit) | DXIL | D3D12 |

Metadata checked by the fixtures:

- triple: `dxil-pc-shadermodelN.M-compute` (or `-library`)
- `"hlsl.shader"="compute"`
- `"hlsl.numthreads"="X,Y,Z"`
- optional before prepare: `"exp-shader"="cs"`

This kit’s D3D12 host uses a conventional DXC root signature (CBV + UAVs).
That is the proven host path. An LDC DirectX backend may additionally use a
wrapper entry and resource intrinsics; that ABI is outside the saxpy host here.
