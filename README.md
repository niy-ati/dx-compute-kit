# dx-compute-kit

Reference DirectX compute path for the [dcompute](https://github.com/libmir/dcompute) / [LDC](https://github.com/ldc-developers/ldc) ecosystem:

**HLSL → DXIL → D3D12 dispatch → CPU-verified saxpy**, plus LLVM IR fixtures for the compute metadata shape expected by an LDC DirectX backend.

> Windows only. Device code in this repository is compiled with [DXC](https://github.com/microsoft/DirectXShaderCompiler). It is **not** a replacement for LDC `-mdcompute-targets=directx-*` codegen.

## About

[dcompute](https://github.com/libmir/dcompute) runs D kernels on CUDA (PTX) and OpenCL (SPIR-V). A DirectX backend needs two halves:

1. **Device** — DXIL (or equivalent) with HLSL-compatible metadata  
2. **Host** — load the blob, bind resources, dispatch, read back, verify  

This repository implements (2) end-to-end using a DXC-built saxpy kernel, and keeps small IR fixtures as a checklist for (1).

It is a **standalone reference**, not official dcompute and not LDC itself.

### Scope

| In scope | Out of scope |
|----------|----------------|
| DXC → DXIL → D3D12 saxpy E2E | LDC `@kernel` → DXIL codegen |
| IR fixtures for `hlsl.shader` / `hlsl.numthreads` / `dxil-pc-shadermodel*-compute` | Graphics / non-compute shaders |
| Documented root signature matching the HLSL registers | Full dcompute-style D host driver |

The D3D12 host uses a conventional root signature (CBV + UAVs). That is separate from LDC’s provisional wrapper / arg-buffer ABI (`*_kernel` + `llvm.dx.resource.*`), which must be debugged against LDC-emitted DXIL.

## Pipeline

```text
kernels/saxpy.hlsl
        │  dxc -T cs_6_0 -E main
        ▼
kernels/out/saxpy.dxil
        │  host/d3d12_saxpy
        ▼
D3D12 upload → PSO → Dispatch → readback
        │
        ▼
CPU verify: Y[i] == a*X[i] + Y0[i]
```

IR fixtures under `fixtures/` are validated by the D CLI and are **not** loaded by D3D12.

## Requirements

- [DUB](https://github.com/dlang/dub) and a D compiler (`ldc2` or `dmd`)
- [DXC](https://github.com/microsoft/DirectXShaderCompiler) (`dxc` on `PATH`, or set `DXC` / pass `--dxc`)
- Visual Studio 2022 Build Tools with the C++ workload and Windows SDK
- A DirectX 12 GPU, or WARP (software adapter)

## Build and run

From the repository root:

```text
run_e2e.bat
```

Or step by step:

```text
dub build
dx-compute-kit --compile-kernel
host\build.bat
dx-compute-kit --e2e
```

Run the host alone after DXIL exists:

```text
host\d3d12_saxpy.exe kernels\out\saxpy.dxil
```

### Expected output

```text
[d3d12-saxpy] adapter: hardware
[d3d12-saxpy] N=1024 a=2.500 mismatches=0 maxAbsErr=0
[d3d12-saxpy] PASS — y = a*x + y verified on D3D12
E2E PASS
```

(`adapter: WARP` is also fine.)

## CLI

```text
dx-compute-kit                  # validate fixtures/*.ll
dx-compute-kit --compile-kernel # HLSL → kernels/out/saxpy.dxil
dx-compute-kit --e2e            # compile (if needed) + run D3D12 host + fixtures
dx-compute-kit --help
```

| Option | Description |
|--------|-------------|
| `--fixtures DIR` | Fixture directory (default: `./fixtures`) |
| `--kernels DIR` | Kernel directory (default: `./kernels`) |
| `--dxc PATH` | Path to `dxc` |
| `--quiet`, `-q` | Less logging |

Exit code `0` on success.

## Layout

```text
source/           D CLI and IR validator
fixtures/         LLVM IR acceptance cases (positive + negative)
kernels/          HLSL saxpy kernel; generated DXIL under kernels/out/
host/             D3D12 saxpy host (C++)
docs/             Backend map and notes
run_e2e.bat       Full Windows pipeline
```

| Path | Role |
|------|------|
| `kernels/saxpy.hlsl` | `y = a*x + y` (`cs_6_0`, `b0` / `u0` / `u1`) |
| `host/d3d12_saxpy.cpp` | Upload, dispatch, readback, CPU verify |
| `fixtures/minimal_cs.ll` | Compute entry attrs (must pass) |
| `fixtures/ldc_wrapper_attrs.ll` | Attrs on `*_kernel` wrapper (must pass) |
| `fixtures/bad_missing_attrs.ll` | Missing attrs (must fail) |

## Kernel and root signature

`kernels/saxpy.hlsl`:

- Entry `main`, `[numthreads(64, 1, 1)]`
- `cbuffer Constants : register(b0)` — `float a`, `uint n`
- `RWStructuredBuffer<float> X : register(u0)`
- `RWStructuredBuffer<float> Y : register(u1)`

Host defaults: `N = 1024`, `a = 2.5`. Root parameters:

| Index | Type | Register |
|-------|------|----------|
| 0 | CBV | `b0` |
| 1 | UAV | `u0` |
| 2 | UAV | `u1` |

## IR fixtures

A fixture passes when it has:

1. `target triple` with `dxil-pc-shadermodel` and `-compute` or `-library`
2. `"hlsl.shader"="compute"`
3. `"hlsl.numthreads"="…"`

Files named `bad_*` are expected to fail. See `docs/BACKEND_MAP.md` for how this relates to CUDA / OpenCL / Vulkan / Metal.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `dub` cannot find the package | Run from the repository root |
| `dxc` not found | Install DXC or set `DXC` / `--dxc` |
| `vcvars64.bat` missing | Install VS 2022 Build Tools + Windows SDK |
| `d3d12_saxpy.exe` missing | Run `host\build.bat` |
| `CreateComputePipelineState` fails | Recompile with DXC; confirm DXBC/DXIL container |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Prefer small, single-concern changes.

## License

MIT — see [LICENSE](LICENSE).

## See also

- [libmir/dcompute](https://github.com/libmir/dcompute) — CUDA / OpenCL dcompute
- [ldc-developers/ldc](https://github.com/ldc-developers/ldc) — LLVM D compiler
- [LLVM DirectX target](https://llvm.org/docs/DirectXUsage.html) — experimental DXIL backend
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) — DXC
