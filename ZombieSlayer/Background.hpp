#pragma once
#include <string>
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// [Background 컴포넌트]
// 텍스처를 화면 전체에 까는 배경. 단위 행렬을 b0에 바인딩해 NDC [-1,1] 쿼드를
// 그대로 채운다(화면비 보정·스케일 없음 → 항상 화면을 꽉 채움).
// Playing 렌더의 가장 앞(클리어 직후, world보다 먼저) 단계에서 그린다.
//
// 텍스처 로드 실패 시(파일 없음 등) 아무것도 그리지 않음(안전).
class Background : public Component {
public:
    Background(ShaderSet texShader, const std::wstring& imagePath)
        : m_shader(texShader), m_path(imagePath) {}

    ~Background() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_material;
    }

    void Start() override {
        ID3D11Device* dev = GraphicsContext::Get()->Device;

        m_loaded = m_tex.Load(dev, m_path);
        if (m_loaded) m_tex.CreateSampler(dev);
        else LOG_ERROR("Background: 텍스처 로드 실패 (%ws)", m_path.c_str());

        m_mesh.Create(CreateTexturedQuad(1.0f, 1.0f)); // NDC 전체를 덮는 쿼드
        m_material = new TextureMaterial(m_shader, &m_tex);

        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        dev->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_loaded || !m_cBuffer) return;

        GraphicsContext* gfx = GraphicsContext::Get();
        m_material->Bind();

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(XMMatrixIdentity()); // 화면 고정
        gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

        UINT stride = sizeof(Vertex), offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_mesh.vBuffer, &stride, &offset);
        gfx->ImmediateContext->Draw(m_mesh.vertexCount, 0);
    }

    void Input()          override {}
    void Update(float dt) override {}

private:
    ShaderSet        m_shader;
    std::wstring     m_path;
    Texture          m_tex;
    Mesh             m_mesh;
    TextureMaterial* m_material = nullptr;
    ID3D11Buffer*    m_cBuffer  = nullptr;
    bool             m_loaded   = false;
};
