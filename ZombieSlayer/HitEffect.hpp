#pragma once
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// [HitEffect 컴포넌트]
// 피격 시 화면 가장자리 4면에 빨간 반투명 쿼드를 표시하고 서서히 fade out.
// UIManager 소유 오브젝트에 부착. HitListener가 Trigger()를 호출한다.
class HitEffect : public Component {
public:
    HitEffect() {}

    ~HitEffect() {
        if (m_cBuffer)  m_cBuffer->Release();
        if (m_blendOn)  m_blendOn->Release();
        if (m_blendOff) m_blendOff->Release();
        delete m_material;
        m_shaders.Release();
    }

    // 피격 시 외부에서 호출
    void Trigger() { m_alpha = 0.6f; }

    void Start() override {
        // uiGradient.hlsl 자체 컴파일 (vertex.a × tintColor.a 그라데이션 PS)
        D3D11_INPUT_ELEMENT_DESC ied[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        m_shaders = GraphicsContext::Get()->CompileAndCreate(L"uiGradient.hlsl", 0, true, ied, 2);

        m_edgeMesh.Create(BuildEdgeVertices());

        // 빨간 머테리얼 (alpha는 Render마다 UpdateSubresource로 갱신)
        m_material = new ColorMaterial(m_shaders, XMFLOAT4(0.8f, 0.0f, 0.0f, 0.0f));

        // 월드 행렬용 상수 버퍼 (단위 행렬 고정)
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage     = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);

        // 알파 블렌딩 ON 상태
        D3D11_BLEND_DESC bdOn = {};
        bdOn.RenderTarget[0].BlendEnable           = TRUE;
        bdOn.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        bdOn.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        bdOn.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        bdOn.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        bdOn.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        bdOn.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        bdOn.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        GraphicsContext::Get()->Device->CreateBlendState(&bdOn, &m_blendOn);

        // 알파 블렌딩 OFF 상태 (렌더 후 복원용)
        D3D11_BLEND_DESC bdOff = {};
        bdOff.RenderTarget[0].BlendEnable           = FALSE;
        bdOff.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        GraphicsContext::Get()->Device->CreateBlendState(&bdOff, &m_blendOff);
    }

    void Update(float dt) override {
        if (m_alpha <= 0.0f) return;
        m_alpha -= FADE_SPEED * dt;
        if (m_alpha < 0.0f) m_alpha = 0.0f;
    }

    void Render() override {
        if (m_alpha <= 0.0f || !m_cBuffer) return;

        GraphicsContext* gfx = GraphicsContext::Get();

        // 알파 블렌딩 ON
        float blendFactor[4] = { 0,0,0,0 };
        gfx->ImmediateContext->OMSetBlendState(m_blendOn, blendFactor, 0xFFFFFFFF);

        // 머테리얼 색상에 현재 alpha 반영
        m_material->SetColor(XMFLOAT4(0.8f, 0.0f, 0.0f, m_alpha));
        m_material->Bind();

        // 화면 고정 — 단위 행렬
        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(XMMatrixIdentity());
        gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

        UINT stride = sizeof(Vertex), offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_edgeMesh.vBuffer, &stride, &offset);
        gfx->ImmediateContext->Draw(m_edgeMesh.vertexCount, 0);

        // 알파 블렌딩 OFF (다른 오브젝트에 영향 없도록 복원)
        gfx->ImmediateContext->OMSetBlendState(m_blendOff, blendFactor, 0xFFFFFFFF);
    }

    void Input() override {}

private:
    static constexpr float FADE_SPEED = 1.0f;   // 초당 alpha 감소량 (0.6 → 0까지 약 0.5초)

    ShaderSet       m_shaders;
    Mesh            m_edgeMesh;
    ColorMaterial*  m_material  = nullptr;
    ID3D11Buffer*   m_cBuffer   = nullptr;
    ID3D11BlendState* m_blendOn  = nullptr;
    ID3D11BlendState* m_blendOff = nullptr;
    float           m_alpha     = 0.0f;

    // 중앙 → 귀퉁이 그라데이션 메시 (삼각형 팬)
    // 버텍스 알파: 중앙=0(투명), 귀퉁이=1(불투명), 엣지 중점=0(투명)
    // PS에서 tintColor.a * vertex.a 로 최종 알파 결정
    std::vector<Vertex> BuildEdgeVertices() {
        XMFLOAT4 transparent = { 1,1,1,0 };  // 중앙 / 엣지 중점
        XMFLOAT4 opaque      = { 1,1,1,1 };  // 귀퉁이

        // 중앙 정점
        Vertex center = { {0.0f, 0.0f, 0.0f}, transparent };

        // 8개 외곽 정점 (시계 방향): 귀퉁이↔엣지 중점 교대
        // 귀퉁이 = 불투명(1), 엣지 중점 = 투명(0)
        struct P { float x, y; bool isCorner; };
        P pts[8] = {
            { -1.0f,  1.0f, true  },   // TL 귀퉁이
            {  0.0f,  1.0f, false },   // TC 엣지 중점
            {  1.0f,  1.0f, true  },   // TR 귀퉁이
            {  1.0f,  0.0f, false },   // RC 엣지 중점
            {  1.0f, -1.0f, true  },   // BR 귀퉁이
            {  0.0f, -1.0f, false },   // BC 엣지 중점
            { -1.0f, -1.0f, true  },   // BL 귀퉁이
            { -1.0f,  0.0f, false },   // LC 엣지 중점
        };

        std::vector<Vertex> verts;
        verts.reserve(8 * 3);
        for (int i = 0; i < 8; i++) {
            int j = (i + 1) % 8;
            XMFLOAT4 colI = pts[i].isCorner ? opaque : transparent;
            XMFLOAT4 colJ = pts[j].isCorner ? opaque : transparent;
            verts.push_back(center);
            verts.push_back({ {pts[i].x, pts[i].y, 0.0f}, colI });
            verts.push_back({ {pts[j].x, pts[j].y, 0.0f}, colJ });
        }
        return verts;
    }
};
