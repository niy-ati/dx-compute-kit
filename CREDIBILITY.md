# Notes (not published docs)

Forum / résumé drafts. Do not ship this file as product documentation if you prefer a cleaner tree — optional to delete before public release.

## Résumé line

Author of `dx-compute-kit` — HLSL saxpy → DXIL → D3D12 host with CPU-verified
results, plus IR fixtures for DirectX compute metadata used by LDC DirectX work.

## Forum draft (new thread in General)

**Subject:** dx-compute-kit — HLSL→DXIL→D3D12 saxpy E2E

```
hi,

published a small DirectX compute reference for the dcompute/LDC lane:

  - HLSL saxpy → DXIL (dxc)
  - D3D12 host: dispatch + readback + CPU verify
  - IR fixtures for hlsl.shader / hlsl.numthreads / dxil triple

Windows: run_e2e.bat

repo: <URL>

this is the host + DXIL half; LDC codegen is separate. feedback welcome.
```
