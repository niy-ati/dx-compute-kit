# Credibility notes (personal — do not paste wholesale)

## Resume line (dx-compute-kit)

Author of `dx-compute-kit` — HLSL saxpy → DXIL → D3D12 host with CPU-verified
results (N=1024), plus IR fixtures for the `hlsl.shader` / `hlsl.numthreads`
shape LDC `targetDirectX` needs.

## Forum draft — new thread in General (or Announce if you feel solid)

**Subject:** dx-compute-kit — HLSL→DXIL→D3D12 saxpy E2E (dcompute / DirectX host)

```
hi,

published a small DirectX compute path aimed at the dcompute/LDC lane:

  - HLSL saxpy → DXIL (dxc)
  - D3D12 host: dispatch + readback + CPU verify (PASS on hardware)
  - IR fixtures for the hlsl.shader / hlsl.numthreads / dxil triple shape

one command on Windows: run_e2e.bat

repo: <URL>

this is the host + DXIL half; LDC codegen is separate work. feedback welcome
from people who have hit DXIL / D3D12 compute quirks.
```

Do not reply under the official SAOC announce thread.
