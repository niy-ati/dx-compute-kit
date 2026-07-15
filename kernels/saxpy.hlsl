// SAXPY: y = a*x + y  (same math as dcompute's first CUDA demo kernel)
// Compile: dxc -T cs_6_0 -E main -Fo kernels/out/saxpy.dxil kernels/saxpy.hlsl

cbuffer Constants : register(b0)
{
    float a;
    uint n;
};

RWStructuredBuffer<float> X : register(u0);
RWStructuredBuffer<float> Y : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= n)
        return;
    Y[id.x] = a * X[id.x] + Y[id.x];
}
