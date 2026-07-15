; Minimal DirectX compute IR shape (mirrors llvm/test/CodeGen/DirectX Metadata tests).
; Used as a regression fixture for LDC targetDirectX metadata emission.
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-pc-shadermodel6.6-compute"

define void @entry() #0 {
entry:
  ret void
}

attributes #0 = {
  noinline nounwind
  "exp-shader"="cs"
  "hlsl.numthreads"="8,1,1"
  "hlsl.shader"="compute"
}
