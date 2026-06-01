#pragma once

// [Framework.hpp] 시스템 헤더 및 전역 구조체
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cstdint>
#include <wincodec.h> // WIC: Windows 기본 이미지 디코딩 (PNG/JPG 로드)

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// 1. 공통 데이터 구조체
// uv 추가: 텍스처 매핑용. 기존 색상 메시는 { {pos}, {col} }로 초기화하면 uv는 {0,0}으로
// 자동 채워지므로(집합 초기화) 색상 셰이더(effect.hlsl)에는 영향이 없다.
struct Vertex {
    XMFLOAT3 pos; XMFLOAT4 col; XMFLOAT2 uv;
};

struct ConstantBuffer {
    XMMATRIX matWorld;
};

struct ColorBuffer {
    XMFLOAT4 tintColor;
};

// 2. 셰이더 리소스 묶음
struct ShaderSet {
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;

    ShaderSet() = default;
    ShaderSet(ID3D11VertexShader* v, ID3D11PixelShader* p, ID3D11InputLayout* l)
        : vs(v), ps(p), layout(l) {
    }

    void Release() {
        if (vs) { vs->Release(); vs = nullptr; }
        if (ps) { ps->Release(); ps = nullptr; }
        if (layout) { layout->Release(); layout = nullptr; }
    }
};

// 3. 텍스처 (Lecture09 구조 이식)
//  - WIC로 PNG/JPG를 읽어 GPU 텍스처 + SRV(셰이더가 보는 통로)를 만든다.
//  - 셰이더는 SamplerState(s0)로 UV 좌표의 색을 보간해 읽는다.
//  - 주의: WIC는 COM이라, 사용 전에 CoInitializeEx 호출이 필요(GameLoop::Initialize에서 처리).
class Texture {
public:
    ID3D11ShaderResourceView* pSRV     = nullptr; // 셰이더가 읽는 텍스처 뷰(t0)
    ID3D11SamplerState*       pSampler = nullptr; // 샘플러(s0)
    uint32_t width = 0, height = 0;

    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture() { Release(); }

    void Release() {
        if (pSRV)     { pSRV->Release();     pSRV = nullptr; }
        if (pSampler) { pSampler->Release(); pSampler = nullptr; }
    }

    // 이미지 파일을 로드해 GPU 텍스처/SRV 생성. 성공 시 true.
    bool Load(ID3D11Device* device, const std::wstring& path) {
        Release();

        IWICImagingFactory* wicFactory = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
        if (FAILED(hr)) return false;

        IWICBitmapDecoder* decoder = nullptr;
        hr = wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) { wicFactory->Release(); return false; }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) { decoder->Release(); wicFactory->Release(); return false; }

        hr = frame->GetSize(&width, &height);
        if (FAILED(hr)) { frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

        // DX11이 좋아하는 RGBA 8비트로 변환
        IWICFormatConverter* converter = nullptr;
        hr = wicFactory->CreateFormatConverter(&converter);
        if (FAILED(hr)) { frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

        std::vector<uint8_t> pixels(width * height * 4);
        hr = converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = width; td.Height = height;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = pixels.data();
        init.SysMemPitch = width * 4;

        ID3D11Texture2D* tex2D = nullptr;
        hr = device->CreateTexture2D(&td, &init, &tex2D);
        if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); wicFactory->Release(); return false; }

        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = td.Format;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
        hr = device->CreateShaderResourceView(tex2D, &sd, &pSRV);

        tex2D->Release(); converter->Release(); frame->Release(); decoder->Release(); wicFactory->Release();
        return SUCCEEDED(hr);
    }

    // 기본 선형 샘플러(부드러운 확대/축소, UV 반복)
    void CreateSampler(ID3D11Device* device) {
        D3D11_SAMPLER_DESC d = {};
        d.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        d.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        d.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        d.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        d.ComparisonFunc = D3D11_COMPARISON_NEVER;
        d.MinLOD = 0;
        d.MaxLOD = D3D11_FLOAT32_MAX;
        device->CreateSamplerState(&d, &pSampler);
    }
};

// 텍스처용 사각형(쿼드) 메시 생성 헬퍼.
// pos는 [-hw,hw] x [-hh,hh], uv는 좌상단(0,0)~우하단(1,1).
// 시계방향(CW) 와인딩 — DX11 기본 백페이스 컬링에서 보이도록(프로젝트 공통 규칙).
inline std::vector<Vertex> CreateTexturedQuad(float hw = 1.0f, float hh = 1.0f) {
    XMFLOAT4 w = { 1, 1, 1, 1 };
    Vertex TL = { { -hw,  hh, 0 }, w, { 0, 0 } };
    Vertex TR = { {  hw,  hh, 0 }, w, { 1, 0 } };
    Vertex BR = { {  hw, -hh, 0 }, w, { 1, 1 } };
    Vertex BL = { { -hw, -hh, 0 }, w, { 0, 1 } };
    return { TL, TR, BR,  TL, BR, BL };
}