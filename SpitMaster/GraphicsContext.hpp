#pragma once
#include "Framework.hpp"
#include "Logger.hpp"

class GraphicsContext {
public:
    ID3D11Device*           Device           = nullptr;
    ID3D11DeviceContext*    ImmediateContext = nullptr;
    IDXGISwapChain*         SwapChain        = nullptr;
    ID3D11RenderTargetView* RTV              = nullptr;
    ID3D11BlendState* AlphaBlendState = nullptr;

    HWND hWnd = nullptr;
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
        LOG_INFO("Resize requested: %dx%d", w, h);

        ImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
        if (RTV) { RTV->Release(); RTV = nullptr; }
        HRESULT hr = SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);

        if (FAILED(hr)) {
            LOG_ERROR("ResizeBuffers failed. hr=0x%08X", hr);
            return;
        }

        LOG_INFO("ResizeBuffers succeeded: %dx%d", w, h);

        CreateRTV(w, h);
    }

    void SetFullscreen(bool goFull) {
        IsFullscreen = goFull;
        SwapChain->SetFullscreenState(goFull, nullptr);
    }

    // 현재 창의 실제 클라이언트 영역 기준 화면 비율 반환 (현재 C키에 의해 buffer 바뀔때만 적용됨)
    float GetAspectRatio()
    {
        RECT rect;
        GetClientRect(hWnd, &rect);

        float width = (float)(rect.right - rect.left);
        float height = (float)(rect.bottom - rect.top);

        if (height <= 0.0f) return 1.0f;

        return width / height;
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
        if (AlphaBlendState)  { AlphaBlendState->Release();  AlphaBlendState  = nullptr; }
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
        this->hWnd = hWnd;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &SwapChain, &Device, nullptr, &ImmediateContext);

        if (FAILED(hr)) return false;

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        Device->CreateBlendState(&blendDesc, &AlphaBlendState);
        return CreateRTV(w, h);
    }

    static GraphicsContext* s_instance;
};
