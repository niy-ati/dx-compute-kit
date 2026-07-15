# dx-compute-kit

**Local path on this machine:**

```text
C:\Users\niyat\Downloads\resume d lang
```

Package / binary name: **`dx-compute-kit`** (see `dub.json`).

---

## What this is (one sentence)

A Windows DirectX compute reference kit for the **dcompute / LDC** lane: **HLSL → DXIL → D3D12 dispatch → CPU-verified results**, plus small LLVM IR fixtures that check the **metadata shape** a future LDC `targetDirectX` must emit.

## What this is *not*

| This kit is not… | Clarification |
|------------------|---------------|
| LDC itself | Kernel device code here is compiled with **DXC**, not `ldc2 -mdcompute-targets=…` |
| Official dcompute | Upstream lives in LDC / dcompute; this is a **standalone reference** |
| A graphics API wrapper | Compute only (one saxpy kernel + host) |
| Related to `dub-watch` | DUB tooling lives elsewhere under `Downloads\d lang\dub-watch` |
| Proof that LDC’s arg-buffer wrapper ABI is done | The D3D12 host here uses a **normal DXC root signature** (CBV + UAVs). LDC’s provisional `*_kernel` + `dx.RawBuffer` ABI is a **separate** problem and is still being debugged |

If you only remember one distinction: **this repo proves the host + DXIL half** of a DirectX dcompute backend. Compiler codegen for D `@kernel` → DXIL is mentored / upstream work.

---

## Why it exists

dcompute already targets CUDA (PTX) and OpenCL (SPIR-V). Vulkan and Metal backends are separate GSoC tracks. DirectX needs:

1. **Device:** DXIL (or equivalent) with the right HLSL-shaped metadata  
2. **Host:** something that can load that blob, bind resources, `Dispatch`, read back, and verify

This kit implements (2) end-to-end with a DXC-built DXIL blob, and keeps small IR fixtures for (1)’s metadata checklist.

---

## End-to-end pipeline (what actually runs)

```text
kernels/saxpy.hlsl
        │
        │  dxc -T cs_6_0 -E main
        ▼
kernels/out/saxpy.dxil     (+ optional saxpy.ll asm dump)
        │
        │  host/d3d12_saxpy.exe
        ▼
D3D12: upload X,Y + constants → PSO → Dispatch → readback Y
        │
        ▼
CPU check: Y[i] == a*X[i] + Y0[i]   (N=1024, a=2.5)
        │
        ▼
PASS  or  FAIL (mismatches / maxAbsErr printed)
```

In parallel, the D CLI walks `fixtures/*.ll` and validates IR shape (positive + negative cases).

---

## Quick start (Windows)

### Requirements

| Tool | Why |
|------|-----|
| D compiler (`ldc2` or `dmd`) + **`dub`** | Build `dx-compute-kit.exe` |
| [DXC](https://github.com/microsoft/DirectXShaderCompiler) (`dxc` on `PATH`, or set `DXC=…\dxc.exe`) | Compile HLSL → DXIL |
| VS 2022 **Build Tools** + Windows SDK | Build `host\d3d12_saxpy.exe` via `cl` |
| DirectX 12 capable GPU **or** WARP | Host prefers hardware; falls back to WARP |

Open a shell in the kit root:

```text
cd /d "C:\Users\niyat\Downloads\resume d lang"
```

### One command

```text
run_e2e.bat
```

That script (from this folder):

1. `dub build`  
2. `dx-compute-kit.exe --compile-kernel`  
3. `host\build.bat`  
4. runs `host\d3d12_saxpy.exe kernels\out\saxpy.dxil`  
5. re-runs fixture checks  

### Expected success (high signal)

You should see lines like:

```text
[d3d12-saxpy] adapter: hardware    (or WARP)
[d3d12-saxpy] N=1024 a=2.500 mismatches=0 maxAbsErr=0
[d3d12-saxpy] PASS — y = a*x + y verified on D3D12
E2E PASS
```

If step 4 fails, the kit is *not* announce-ready for the host claim — fix DXC / SDK / DXIL load before anything else.

### Step by step (same as the bat)

```text
dub build
.\dx-compute-kit.exe --compile-kernel
host\build.bat
.\dx-compute-kit.exe --e2e
```

Or run the host alone once DXIL exists:

```text
host\d3d12_saxpy.exe kernels\out\saxpy.dxil
```

---

## CLI (`dx-compute-kit.exe`)

| Invokation | Behaviour |
|------------|-----------|
| *(no flags)* | Validate every `fixtures\*.ll` (default) |
| `--compile-kernel` | DXC: `kernels\saxpy.hlsl` → `kernels\out\saxpy.dxil` (+ try asm dump) |
| `--e2e` | Compile kernel if needed, then run `host\d3d12_saxpy.exe`, then still validate fixtures |
| `--fixtures DIR` | Override fixtures directory |
| `--kernels DIR` | Override kernels directory |
| `--dxc PATH` | Override `dxc` (else env `DXC`, then common winget path, then `dxc`) |
| `--quiet` / `-q` | Less logging |
| `--help` | Options |

Exit code `0` = all checks OK (and E2E host OK if `--e2e`). Non-zero = something failed.

---

## Repository layout

```text
C:\Users\niyat\Downloads\resume d lang\
│
├── README.md                 ← this file
├── CREDIBILITY.md            ← private draft for forum / résumé (not product docs)
├── CONTRIBUTING.md           ← change discipline for this tree
├── LICENSE                   ← MIT
├── dub.json                  ← DUB package: name = dx-compute-kit
├── run_e2e.bat               ← full pipeline; must be run from this folder
│
├── source\
│   ├── app.d                 ← CLI, DXC invoke, fixture loop, --e2e
│   └── validate_ir.d         ← DirectX compute IR shape checks
│
├── fixtures\                 ← LLVM IR acceptance tests (text .ll)
│   ├── minimal_cs.ll         ← Clang/LLVM-style compute entry attrs
│   ├── ldc_wrapper_attrs.ll  ← LDC-style attrs on *_kernel wrapper
│   └── bad_missing_attrs.ll  ← negative: must FAIL validation
│
├── kernels\
│   ├── saxpy.hlsl            ← device: y = a*x + y
│   └── out\                  ← generated (gitignored-friendly)
│       ├── saxpy.dxil        ← DXBC/DXIL blob for D3D12 PSO
│       └── saxpy.ll          ← optional dxc asm dump
│
├── host\
│   ├── d3d12_saxpy.cpp       ← D3D12 compute host (oracle)
│   ├── build.bat             ← vcvars64 + cl → d3d12_saxpy.exe
│   └── d3d12_saxpy.exe       ← built binary
│
└── docs\
    ├── BACKEND_MAP.md        ← CUDA/OCL/Vulkan/Metal vs DirectX
    └── D3D12_HOST_NEXT.md    ← optional follow-ups (not required for PASS)
```

Generated binaries (`.exe`, `.dxil`, `.obj`, `.dub/`) are build products; rebuild with `run_e2e.bat` if missing.

---

## Device kernel (what the GPU runs)

File: `kernels/saxpy.hlsl`

- Entry: `main`, profile `cs_6_0`, `[numthreads(64,1,1)]`
- `cbuffer` `Constants` at `b0`: `float a`, `uint n`
- `RWStructuredBuffer<float> X` at `u0`
- `RWStructuredBuffer<float> Y` at `u1`
- Body: `Y[i] = a * X[i] + Y[i]` with bounds check on `n`

Host uses **N = 1024**, **a = 2.5f**, fills `X`/`Y`, dispatches, then verifies every element on the CPU.

Root signature (matches the HLSL registers):

| Slot | Type | Register |
|------|------|----------|
| 0 | CBV | `b0` |
| 1 | UAV | `u0` (X) |
| 2 | UAV | `u1` (Y) |

---

## Host (`host/d3d12_saxpy.cpp`) — behaviour contract

1. Read DXIL/DXBC file from argv[1] (default `kernels/out/saxpy.dxil`)
2. Create D3D12 device (first hardware adapter; else WARP)
3. Build root signature as above
4. `CreateComputePipelineState` from the byte blob  
5. Upload constants + `X` + `Y`
6. `Dispatch` enough groups for N with 64 threads/group  
7. Read back `Y`, compare to CPU reference  
8. Print `mismatches` / `maxAbsErr`; exit 0 only on PASS  

If `CreateComputePipelineState` fails, the DXIL is wrong for the OS runtime / root signature — not an IR-fixture issue.

---

## IR fixtures (what they prove)

Validator: `source/validate_ir.d` → `validateDirectXComputeIR`.

**Required for a “good” fixture:**

1. `target triple` containing `dxil-pc-shadermodel` **and** `-compute` or `-library`  
2. Function attr `"hlsl.shader"="compute"`  
3. Function attr `"hlsl.numthreads"="…"`  

Optional (often present before `dxil-prepare`): `"exp-shader"="cs"`.

| File | Expectation |
|------|-------------|
| `fixtures/minimal_cs.ll` | Must **pass** (Classic compute entry style) |
| `fixtures/ldc_wrapper_attrs.ll` | Must **pass** (attrs on `*_kernel`; core without attrs) |
| `fixtures/bad_missing_attrs.ll` | Must **fail** validation (name prefix `bad_`) |

These fixtures do **not** load into D3D12. They are compile-time / doc-level checks for the **metadata dialect**, aligned with LLVM DirectX / Clang HLSL tests and LDC `targetDirectX` work.

---

## Two different DirectX stories (do not mix them up)

| Path | Device code from | Host binding model | Status in this repo |
|------|------------------|--------------------|---------------------|
| **A. Reference (here)** | DXC on `saxpy.hlsl` | Root CBV + two UAVs | **E2E verified PASS** |
| **B. LDC wrapper ABI** | `ldc2 -mdcompute-targets=directx-…` → `*_kernel` + `llvm.dx.resource.*` | Arg-buffer / RawBuffer style | **Not** this host; debug separately (mentorship harness) |

`fixtures/ldc_wrapper_attrs.ll` only checks **attrs on the wrapper name**. It does not claim LDC DXIL runs in `d3d12_saxpy.exe`.

---

## How this sits among dcompute backends

See `docs/BACKEND_MAP.md`. Summary:

| Backend | Device | Host |
|---------|--------|------|
| CUDA | PTX | CUDA |
| OpenCL | SPIR-V | OpenCL |
| Vulkan | SPIR-V | Vulkan |
| Metal | Metal IR | Metal |
| **DirectX (this kit)** | **DXIL** | **D3D12** |

---

## Troubleshooting

| Symptom | Likely cause | What to do |
|---------|--------------|------------|
| `No package manifest` / dub wrong folder | Ran from `Downloads` instead of kit root | `cd` to `resume d lang` (or use `run_e2e.bat`, which `cd`s to itself) |
| `dxc` not found | DXC not on PATH | Install DXC / set `DXC` / pass `--dxc` |
| `vcvars64.bat not found` | No VS Build Tools | Install VS 2022 Build Tools + C++ + Windows SDK |
| Host missing | Didn’t build host | `host\build.bat` |
| PSO create fails | Bad / incompatible DXIL | Recompile with dxc; confirm magic `DXBC` |
| Fixture FAIL on good file | Edited IR without attrs | Compare to `minimal_cs.ll` |
| Fixture PASS on `bad_*` | Validator regression | Fix `validate_ir.d` |

---

## Maintenance

Keep changes small and reviewable — see [CONTRIBUTING.md](CONTRIBUTING.md).  
Do not reformat the tree “for style.” One concern per change.

## License

MIT — see [LICENSE](LICENSE).

---

## Related work (separate trees)

| Tree | Path | Role |
|------|------|------|
| This kit | `C:\Users\niyat\Downloads\resume d lang` | DXIL + D3D12 host reference |
| LDC DirectX scaffold | mentorship fork `targetDirectX-scaffold` | Compiler attrs / wrapper IR |
| LDC D3D12 ABI harness | `ldc/tests/dcompute/directx_harness` | Debug LDC-emitted DXIL + arg buffer |
| dub-watch | `C:\Users\niyat\Downloads\d lang\dub-watch` | Stock-dub watch tooling (unrelated) |
"# dx-compute-kit" 
