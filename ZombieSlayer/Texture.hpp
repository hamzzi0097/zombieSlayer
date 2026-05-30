#pragma once
#include "Framework.hpp"
#include <wincodec.h>
#include <cstdint>

class Texture
{
public:
    ID3D11ShaderResourceView* pSRV = nullptr;
    ID3D11SamplerState* pSampler = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;

    Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    ~Texture()
    {
        Release();
    }

    void Release()
    {
        if (pSRV)
        {
            pSRV->Release();
            pSRV = nullptr;
        }

        if (pSampler)
        {
            pSampler->Release();
            pSampler = nullptr;
        }
    }

    bool Load(ID3D11Device* device, const std::wstring& path)
    {
        Release();

        IWICImagingFactory* wicFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory)
        );

        if (FAILED(hr))
            return false;

        IWICBitmapDecoder* decoder = nullptr;
        hr = wicFactory->CreateDecoderFromFilename(
            path.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder
        );

        if (FAILED(hr))
        {
            wicFactory->Release();
            return false;
        }

        IWICBitmapFrameDecode* frame = nullptr;
        hr = decoder->GetFrame(0, &frame);

        if (FAILED(hr))
        {
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        hr = frame->GetSize(&width, &height);

        if (FAILED(hr))
        {
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        IWICFormatConverter* converter = nullptr;
        hr = wicFactory->CreateFormatConverter(&converter);

        if (FAILED(hr))
        {
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        hr = converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
        );

        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        std::vector<uint8_t> pixelData(width * height * 4);

        hr = converter->CopyPixels(
            nullptr,
            width * 4,
            (UINT)pixelData.size(),
            pixelData.data()
        );

        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixelData.data();
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* tex2D = nullptr;
        hr = device->CreateTexture2D(&texDesc, &initData, &tex2D);

        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            wicFactory->Release();
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(tex2D, &srvDesc, &pSRV);

        tex2D->Release();
        converter->Release();
        frame->Release();
        decoder->Release();
        wicFactory->Release();

        if (FAILED(hr))
            return false;

        CreateSampler(device);

        return true;
    }

    void CreateSampler(ID3D11Device* device)
    {
        if (pSampler)
        {
            pSampler->Release();
            pSampler = nullptr;
        }

        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;

        device->CreateSamplerState(&desc, &pSampler);
    }
};