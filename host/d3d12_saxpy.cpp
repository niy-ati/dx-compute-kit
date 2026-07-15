// d3d12_saxpy.cpp — D3D12 compute host for kernels/out/saxpy.dxil
// Reference path for a future dcompute DirectX driver: upload → dispatch → readback → verify.
// Build: host\build.bat (VS Build Tools + Windows SDK).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

static void die(const char* msg, HRESULT hr = S_OK)
{
    if (FAILED(hr))
        std::fprintf(stderr, "error: %s (HRESULT=0x%08X)\n", msg, (unsigned)hr);
    else
        std::fprintf(stderr, "error: %s\n", msg);
    std::exit(1);
}

static void check(HRESULT hr, const char* what)
{
    if (FAILED(hr))
        die(what, hr);
}

static std::vector<uint8_t> readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        die(("cannot open " + path).c_str());
    return std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
}

struct alignas(256) Constants
{
    float a;
    uint32_t n;
    float pad[62];
};

static constexpr UINT N = 1024;
static constexpr float A = 2.5f;

static void transition(
    ID3D12GraphicsCommandList* list,
    ID3D12Resource* res,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = res;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = after;
    list->ResourceBarrier(1, &b);
}

static void waitGpu(ID3D12CommandQueue* queue, ID3D12Fence* fence, UINT64& value, HANDLE event)
{
    const UINT64 signal = ++value;
    check(queue->Signal(fence, signal), "Signal");
    if (fence->GetCompletedValue() < signal)
    {
        check(fence->SetEventOnCompletion(signal, event), "SetEventOnCompletion");
        WaitForSingleObject(event, INFINITE);
    }
}

int main(int argc, char** argv)
{
    std::string dxilPath = "kernels/out/saxpy.dxil";
    if (argc >= 2)
        dxilPath = argv[1];

    auto shader = readFile(dxilPath);
    std::printf("[d3d12-saxpy] DXIL %s (%zu bytes)\n", dxilPath.c_str(), shader.size());

    check(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "CoInitializeEx");

    ComPtr<IDXGIFactory4> factory;
    check(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");

    ComPtr<ID3D12Device> device;
    bool haveDevice = false;
    for (UINT i = 0;; ++i)
    {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        {
            haveDevice = true;
            std::printf("[d3d12-saxpy] adapter: hardware\n");
            break;
        }
    }
    if (!haveDevice)
    {
        ComPtr<IDXGIAdapter> warp;
        check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)), "EnumWarpAdapter");
        check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)),
              "D3D12CreateDevice(WARP)");
        std::printf("[d3d12-saxpy] adapter: WARP\n");
    }

    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC qdesc{};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    check(device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&queue)), "CreateCommandQueue");

    ComPtr<ID3D12CommandAllocator> alloc;
    check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)),
          "CreateCommandAllocator");

    ComPtr<ID3D12GraphicsCommandList> cmd;
    check(device->CreateCommandList(
              0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&cmd)),
          "CreateCommandList");

    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[1].Descriptor.ShaderRegister = 0;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[2].Descriptor.ShaderRegister = 1;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 3;
    rsDesc.pParameters = params;

    ComPtr<ID3DBlob> rsBlob;
    ComPtr<ID3DBlob> rsErr;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsErr);
    if (FAILED(hr))
    {
        if (rsErr)
            std::fprintf(stderr, "%s\n", (const char*)rsErr->GetBufferPointer());
        die("D3D12SerializeRootSignature", hr);
    }

    ComPtr<ID3D12RootSignature> rootSig;
    check(device->CreateRootSignature(
              0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig)),
          "CreateRootSignature");

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSig.Get();
    psoDesc.CS.pShaderBytecode = shader.data();
    psoDesc.CS.BytecodeLength = shader.size();
    ComPtr<ID3D12PipelineState> pso;
    check(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)),
          "CreateComputePipelineState");

    const UINT64 floatBytes = sizeof(float) * N;
    const UINT64 cbBytes = sizeof(Constants);

    auto makeBuf = [&](UINT64 size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_FLAGS flags,
                       D3D12_RESOURCE_STATES state, ComPtr<ID3D12Resource>& out) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = heap;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = size;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rd.Flags = flags;
        check(device->CreateCommittedResource(
                  &hp, D3D12_HEAP_FLAG_NONE, &rd, state, nullptr, IID_PPV_ARGS(&out)),
              "CreateCommittedResource");
    };

    ComPtr<ID3D12Resource> bufX, bufY, upload, readback, cbUpload;
    makeBuf(floatBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON, bufX);
    makeBuf(floatBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON, bufY);
    makeBuf(floatBytes * 2, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ, upload);
    makeBuf(floatBytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_DEST, readback);
    makeBuf(cbBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ, cbUpload);

    std::vector<float> hostX(N), hostY(N), expected(N);
    for (UINT i = 0; i < N; ++i)
    {
        hostX[i] = float(i) * 0.5f;
        hostY[i] = float(i) * 0.25f;
        expected[i] = A * hostX[i] + hostY[i];
    }

    {
        void* p = nullptr;
        check(upload->Map(0, nullptr, &p), "upload Map");
        std::memcpy(p, hostX.data(), floatBytes);
        std::memcpy(static_cast<char*>(p) + floatBytes, hostY.data(), floatBytes);
        upload->Unmap(0, nullptr);
    }
    {
        Constants c{};
        c.a = A;
        c.n = N;
        void* p = nullptr;
        check(cbUpload->Map(0, nullptr, &p), "cb Map");
        std::memcpy(p, &c, sizeof(c));
        cbUpload->Unmap(0, nullptr);
    }

    transition(cmd.Get(), bufX.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    transition(cmd.Get(), bufY.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmd->CopyBufferRegion(bufX.Get(), 0, upload.Get(), 0, floatBytes);
    cmd->CopyBufferRegion(bufY.Get(), 0, upload.Get(), floatBytes, floatBytes);
    transition(cmd.Get(), bufX.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    transition(cmd.Get(), bufY.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cmd->SetPipelineState(pso.Get());
    cmd->SetComputeRootSignature(rootSig.Get());
    cmd->SetComputeRootConstantBufferView(0, cbUpload->GetGPUVirtualAddress());
    cmd->SetComputeRootUnorderedAccessView(1, bufX->GetGPUVirtualAddress());
    cmd->SetComputeRootUnorderedAccessView(2, bufY->GetGPUVirtualAddress());
    cmd->Dispatch((N + 63) / 64, 1, 1);

    transition(cmd.Get(), bufY.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmd->CopyBufferRegion(readback.Get(), 0, bufY.Get(), 0, floatBytes);

    check(cmd->Close(), "Close");
    ID3D12CommandList* lists[] = { cmd.Get() };
    queue->ExecuteCommandLists(1, lists);

    ComPtr<ID3D12Fence> fence;
    check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "CreateFence");
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event)
        die("CreateEventW");
    UINT64 fenceValue = 0;
    waitGpu(queue.Get(), fence.Get(), fenceValue, event);
    CloseHandle(event);

    std::vector<float> got(N);
    {
        void* p = nullptr;
        check(readback->Map(0, nullptr, &p), "readback Map");
        std::memcpy(got.data(), p, floatBytes);
        readback->Unmap(0, nullptr);
    }

    int mismatches = 0;
    float maxAbs = 0.f;
    for (UINT i = 0; i < N; ++i)
    {
        const float err = std::fabs(got[i] - expected[i]);
        if (err > maxAbs)
            maxAbs = err;
        if (err > 1e-4f)
            ++mismatches;
    }

    std::printf("[d3d12-saxpy] N=%u a=%.3f mismatches=%d maxAbsErr=%.6g\n", N, A, mismatches, maxAbs);
    if (mismatches != 0)
    {
        std::fprintf(stderr, "FAIL: GPU saxpy does not match CPU reference\n");
        return 2;
    }

    std::printf("[d3d12-saxpy] PASS — y = a*x + y verified on D3D12\n");
    return 0;
}
