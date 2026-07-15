; IR shape matching LDC targetDirectX wrapper entry (from dcompute spike).
; Core D body keeps real params; attrs live on *_kernel. Used as an acceptance
; fixture for what LDC emits before D3D12 load.
target datalayout = "e-m:e-ve-p:32:32-i1:32-i8:8-i16:16-i32:32-i64:64-f16:16-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-pc-shadermodel6.6-compute"

define void @kernel_core(ptr addrspace(1) %out) #0 {
  store float 4.200000e+01, ptr addrspace(1) %out, align 4
  ret void
}

define void @kernel_core_kernel() #1 {
  ret void
}

attributes #0 = { "frame-pointer"="all" }
attributes #1 = {
  "exp-shader"="cs"
  "hlsl.numthreads"="8,1,1"
  "hlsl.shader"="compute"
}
