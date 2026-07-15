# D3D12 host — status

Done: `host/d3d12_saxpy.cpp` loads DXC DXIL, dispatches, verifies.

Optional later (separate commits, only if needed):

1. Descriptor-heap SRV/UAV tables (closer to engine-style RS)
2. Thin D wrapper that shells the same C++ host (keep C++ as oracle)
3. Second host path for LDC-emitted `*_kernel` + arg-buffer ABI  
   (that work belongs with the LDC harness, not this DXC saxpy path)

Do not mix those into the IR validator.
