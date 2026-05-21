#pragma once
#include "Framework.hpp"

class GraphicsContext {
public:
    ID3D11Device*           Device           = nullptr;
    ID3D11DeviceContext*    ImmediateContext = nullptr;
    IDXGISwapChain*         SwapChain        = nullptr;
    ID3D11RenderTargetView* RTV              = nullptr;
    bool IsFullscreen = false;
    int  VSync        = 1;

    // -------------------------------------------------------------------------
    static bool Create(HWND hWnd, int w, int h) {
        if (s_instance) return true;
        s_instance = new GraphicsContext();
        if (!s_instance->Init(hWnd, w, h)) {
            delete s_instance;
            s_instance = nullptr;
            return false;
        }
        return true;
    }

    static GraphicsContext* Get() { return s_instance; }

    static void Destroy() {
        delete s_instance;
        s_instance = nullptr;
    }

    GraphicsContext(const GraphicsContext&)            = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    // -------------------------------------------------------------------------
    bool CreateRTV(int w, int h) {
        if (RTV) { RTV->Release(); RTV = nullptr; }
        ID3D11Texture2D* pBB = nullptr;
        SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBB);
        Device->CreateRenderTargetView(pBB, nullptr, &RTV);
        pBB->Release();
        return true;
    }

    void Resize(int w, int h) {
        ImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        if (RTV) { RTV->Release(); RTV = nullptr; }
        SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        CreateRTV(w, h);
    }

    void SetFullscreen(bool goFull) {
        IsFullscreen = goFull;
        SwapChain->SetFullscreenState(goFull, nullptr);
    }

    ShaderSet CompileAndCreate(const void* source, size_t length, bool isFile,
                               D3D11_INPUT_ELEMENT_DESC* ied, UINT iedCount) {
        ShaderSet res;
        ID3DBlob* vsBlob  = nullptr;
        ID3DBlob* psBlob  = nullptr;
        ID3DBlob* errBlob = nullptr;

        auto compile = [&](const char* entry, const char* target, ID3DBlob*& out) -> bool {
            HRESULT hr;
            if (isFile)
                hr = D3DCompileFromFile((LPCWSTR)source, nullptr, nullptr, entry, target, 0, 0, &out, &errBlob);
            else
                hr = D3DCompile(source, length, nullptr, nullptr, nullptr, entry, target, 0, 0, &out, &errBlob);

            if (FAILED(hr)) {
                if (errBlob) {
                    printf("[Shader Error] %s:\n%s\n", entry, (char*)errBlob->GetBufferPointer());
                    errBlob->Release(); errBlob = nullptr;
                }
                return false;
            }
            return true;
        };

        if (!compile("VS", "vs_5_0", vsBlob)) return res;
        if (!compile("PS", "ps_5_0", psBlob)) { vsBlob->Release(); return res; }

        Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &res.vs);
        Device->CreatePixelShader (psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &res.ps);

        if (ied)
            Device->CreateInputLayout(ied, iedCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &res.layout);

        vsBlob->Release();
        psBlob->Release();
        return res;
    }

private:
    GraphicsContext() = default;

    ~GraphicsContext() {
        if (RTV)              { RTV->Release();              RTV              = nullptr; }
        if (SwapChain)        { SwapChain->Release();        SwapChain        = nullptr; }
        if (ImmediateContext) { ImmediateContext->Release(); ImmediateContext = nullptr; }
        if (Device)           { Device->Release();           Device           = nullptr; }
    }

    bool Init(HWND hWnd, int w, int h) {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount          = 1;
        sd.BufferDesc.Width     = w;
        sd.BufferDesc.Height    = h;
        sd.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow         = hWnd;
        sd.SampleDesc.Count     = 1;
        sd.Windowed             = TRUE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &SwapChain, &Device, nullptr, &ImmediateContext);

        if (FAILED(hr)) return false;
        return CreateRTV(w, h);
    }

    static GraphicsContext* s_instance;
};
