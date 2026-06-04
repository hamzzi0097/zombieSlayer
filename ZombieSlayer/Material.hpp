#pragma once
#include "Framework.hpp"

class Material {
public:
    ShaderSet shaders;
    Material(ShaderSet s) : shaders(s) {}
    virtual ~Material() {}
    virtual void Bind() = 0;
};

class ColorMaterial : public Material {
public:
    XMFLOAT4 color;
    ID3D11Buffer* pColorBuffer = nullptr;

    ColorMaterial(ShaderSet s, XMFLOAT4 col)
        : Material(s), color(col) {
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ColorBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &pColorBuffer);
    }

    virtual ~ColorMaterial() {
        if (pColorBuffer) pColorBuffer->Release();
    }

    void SetColor(XMFLOAT4 col) { color = col; }

    void Bind() override {
        ID3D11DeviceContext* context = GraphicsContext::Get()->ImmediateContext;
        context->IASetInputLayout(shaders.layout);
        context->VSSetShader(shaders.vs, nullptr, 0);
        context->PSSetShader(shaders.ps, nullptr, 0);

        ColorBuffer cb = { color };
        context->UpdateSubresource(pColorBuffer, 0, nullptr, &cb, 0, 0);
        context->PSSetConstantBuffers(1, 1, &pColorBuffer);
    }
};

// [TextureMaterial] 텍스처를 입히는 머티리얼.
//  - texture.hlsl(ShaderSet)을 사용하고, t0에 SRV / s0에 샘플러를 바인딩한다.
//  - tint(b1)를 곱해 색조/페이드 조절 가능(기본 흰색이면 원본 그대로).
//  - 월드 행렬(b0)은 MeshRenderer가 바인딩하므로 다른 오브젝트처럼 이동/스케일된다.
//  - Texture 객체의 수명은 외부(생성한 쪽)에서 관리한다(머티리얼은 소유하지 않음).
class TextureMaterial : public Material {
public:
    Texture* texture = nullptr;
    XMFLOAT4 tint;
    ID3D11Buffer* pTintBuffer = nullptr;

    TextureMaterial(ShaderSet s, Texture* tex, XMFLOAT4 tintColor = XMFLOAT4(1, 1, 1, 1))
        : Material(s), texture(tex), tint(tintColor) {
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ColorBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &pTintBuffer);
    }

    virtual ~TextureMaterial() {
        if (pTintBuffer) pTintBuffer->Release();
    }

    void SetTint(XMFLOAT4 c) { tint = c; }
    void SetTexture(Texture* t) { texture = t; }

    void Bind() override {
        ID3D11DeviceContext* context = GraphicsContext::Get()->ImmediateContext;
        context->IASetInputLayout(shaders.layout);
        context->VSSetShader(shaders.vs, nullptr, 0);
        context->PSSetShader(shaders.ps, nullptr, 0);

        ColorBuffer cb = { tint };
        context->UpdateSubresource(pTintBuffer, 0, nullptr, &cb, 0, 0);
        context->PSSetConstantBuffers(1, 1, &pTintBuffer);

        if (texture) {
            context->PSSetShaderResources(0, 1, &texture->pSRV);     // t0
            context->PSSetSamplers(0, 1, &texture->pSampler);        // s0
        }
    }
};