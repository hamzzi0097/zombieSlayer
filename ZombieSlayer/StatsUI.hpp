#pragma once
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Logger.hpp"

// [StatsUI 컴포넌트]
// 화면 상단 좌측: 생존 시간 MM:SS (흰색)
// 화면 상단 우측: 킬 카운트 3자리 (노란색)
// 7-세그먼트 스타일. 로컬 공간 [0..1] x [0..2], 월드 행렬로 NDC 배치.
class StatsUI : public Component
{
public:
    StatsUI(ShaderSet shaders, const int* killCount, const float* playTime)
        : m_shaders(shaders), m_killCount(killCount), m_playTime(playTime) {}

    ~StatsUI() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_matTime;
        delete m_matKill;
    }

    void Start() override {
        for (int i = 0; i < 10; i++)
            m_digitMeshes[i].Create(BuildDigitVertices(i));
        m_colonMesh.Create(BuildColonVertices());

        m_matTime = new ColorMaterial(m_shaders, XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f)); // 흰색
        m_matKill = new ColorMaterial(m_shaders, XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f)); // 노란색

        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_cBuffer) return;

        const float aspect = GraphicsContext::Get()->GetAspectRatio();
        const float DH  = 0.16f;               // 자리수 NDC 높이 (~48px at 600h)
        const float DW  = DH * 0.5f / aspect;  // 자리수 NDC 너비 (1:2 비율 + 화면비)
        const float GAP = DW * 0.35f;           // 자리수 간 간격
        const float STP = DW + GAP;             // 한 칸 이동 폭

        // ── 생존 시간 (상단 좌측) MM:SS ─────────────────────────────────────
        int totalSec = (int)*m_playTime;
        int minutes  = totalSec / 60; if (minutes > 99) minutes = 99;
        int seconds  = totalSec % 60;

        const float TX = -0.95f;
        const float TY =  0.78f;

        m_matTime->Bind();
        DrawDigit(minutes / 10,  TX,          TY, DW, DH);
        DrawDigit(minutes % 10,  TX + STP,    TY, DW, DH);
        DrawColon(               TX + STP*2,  TY, DW, DH);
        DrawDigit(seconds / 10,  TX + STP*3,  TY, DW, DH);
        DrawDigit(seconds % 10,  TX + STP*4,  TY, DW, DH);

        // ── 킬 카운트 (상단 우측) 3자리 우측 정렬 ────────────────────────────
        int kills = *m_killCount; if (kills > 999) kills = 999;

        const float KX_END = 0.95f;
        const float KX     = KX_END - STP * 2.0f - DW; // 세 자리 오른쪽 끝 고정
        const float KY     = 0.78f;

        m_matKill->Bind();
        DrawDigit(kills / 100,       KX,         KY, DW, DH);
        DrawDigit((kills / 10) % 10, KX + STP,   KY, DW, DH);
        DrawDigit(kills % 10,        KX + STP*2, KY, DW, DH);
    }

    void Update(float dt) override {}
    void Input()           override {}

private:
    ShaderSet      m_shaders;
    const int*     m_killCount = nullptr;
    const float*   m_playTime  = nullptr;

    Mesh           m_digitMeshes[10];
    Mesh           m_colonMesh;
    ColorMaterial* m_matTime    = nullptr;
    ColorMaterial* m_matKill    = nullptr;
    ID3D11Buffer*  m_cBuffer    = nullptr;

    // ── 7세그먼트 ────────────────────────────────────────────────────────────
    // 로컬 공간 [0..1] x [0..2], 세그먼트 두께 T
    static constexpr float T = 0.22f;

    struct Seg { float x0, y0, x1, y1; };

    // 세그먼트 7개 반환 (인덱스: 0=a상가로, 1=b우상세로, 2=c우하세로,
    //                           3=d하가로, 4=e좌하세로, 5=f좌상세로, 6=g중가로)
    static Seg GetSeg(int i) {
        switch (i) {
        case 0: return {    T,  2.f-T, 1.f-T, 2.f      }; // a 상단 가로
        case 1: return { 1.f-T, 1.f,   1.f,   2.f-T    }; // b 우상 세로
        case 2: return { 1.f-T, 0.f,   1.f,   1.f      }; // c 우하 세로
        case 3: return {    T,  0.f,   1.f-T, T         }; // d 하단 가로
        case 4: return {  0.f,  0.f,   T,     1.f       }; // e 좌하 세로
        case 5: return {  0.f,  1.f,   T,     2.f-T     }; // f 좌상 세로
        case 6: return {    T,  1.f-T*.5f, 1.f-T, 1.f+T*.5f }; // g 중간 가로
        default: return {};
        }
    }

    // 숫자 d의 세그먼트 비트마스크 반환 (bit0=a, bit1=b, ..., bit6=g)
    static int GetMask(int d) {
        switch (d) {
        case 0: return 0x3F; // a,b,c,d,e,f
        case 1: return 0x06; // b,c
        case 2: return 0x5B; // a,b,d,e,g
        case 3: return 0x4F; // a,b,c,d,g
        case 4: return 0x66; // b,c,f,g
        case 5: return 0x6D; // a,c,d,f,g
        case 6: return 0x7D; // a,c,d,e,f,g
        case 7: return 0x07; // a,b,c
        case 8: return 0x7F; // 전체
        case 9: return 0x6F; // a,b,c,d,f,g
        default: return 0;
        }
    }

    // 직사각형 → 삼각형 2개 (6 정점)
    // [중요] 시계방향(CW) 와인딩으로 생성한다.
    //   DX11 기본 래스터라이저(FrontCounterClockwise=FALSE, CullMode=BACK)는
    //   CW를 '앞면'으로 보고 CCW(뒷면)는 컬링한다. HeartUI/BombCooldownUI가
    //   화면에 보였던 이유도 그 메시들이 CW였기 때문. 여기서도 같은 규칙을 따른다.
    //   순서: BL → TR → BR (삼각형1),  BL → TL → TR (삼각형2)
    static void PushRect(std::vector<Vertex>& v, const Seg& s) {
        XMFLOAT4 w = { 1,1,1,1 };
        v.push_back({ {s.x0, s.y0, 0}, w }); v.push_back({ {s.x1, s.y1, 0}, w }); v.push_back({ {s.x1, s.y0, 0}, w });
        v.push_back({ {s.x0, s.y0, 0}, w }); v.push_back({ {s.x0, s.y1, 0}, w }); v.push_back({ {s.x1, s.y1, 0}, w });
    }

    std::vector<Vertex> BuildDigitVertices(int d) {
        std::vector<Vertex> v;
        int mask = GetMask(d);
        for (int i = 0; i < 7; i++)
            if (mask & (1 << i)) PushRect(v, GetSeg(i));

        if (v.empty()) { // 안전 장치: 빈 메시 방지 (Draw(0) 회피)
            XMFLOAT4 z = {};
            v.push_back({ {0,0,0}, z }); v.push_back({ {0,0,0}, z }); v.push_back({ {0,0,0}, z });
        }
        return v;
    }

    // 콜론: 위아래 두 개의 작은 점
    std::vector<Vertex> BuildColonVertices() {
        std::vector<Vertex> v;
        constexpr float R = 0.14f;
        PushRect(v, { 0.5f-R, 0.5f-R, 0.5f+R, 0.5f+R }); // 하단 점
        PushRect(v, { 0.5f-R, 1.5f-R, 0.5f+R, 1.5f+R }); // 상단 점
        return v;
    }

    // 월드 행렬 설정: 로컬 [0..1]x[0..2] → NDC [px..px+dw] x [py..py+dh]
    void SetWorld(float px, float py, float dw, float dh) {
        XMMATRIX world = XMMatrixScaling(dw, dh * 0.5f, 1.0f)
                       * XMMatrixTranslation(px, py, 0.0f);
        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(world);
        auto* ctx = GraphicsContext::Get()->ImmediateContext;
        ctx->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        ctx->VSSetConstantBuffers(0, 1, &m_cBuffer);
    }

    void DrawDigit(int d, float px, float py, float dw, float dh) {
        if (d < 0 || d > 9 || !m_digitMeshes[d].vBuffer) return;
        SetWorld(px, py, dw, dh);
        auto* ctx = GraphicsContext::Get()->ImmediateContext;
        UINT stride = sizeof(Vertex), offset = 0;
        ctx->IASetVertexBuffers(0, 1, &m_digitMeshes[d].vBuffer, &stride, &offset);
        ctx->Draw(m_digitMeshes[d].vertexCount, 0);
    }

    void DrawColon(float px, float py, float dw, float dh) {
        if (!m_colonMesh.vBuffer) return;
        SetWorld(px, py, dw, dh);
        auto* ctx = GraphicsContext::Get()->ImmediateContext;
        UINT stride = sizeof(Vertex), offset = 0;
        ctx->IASetVertexBuffers(0, 1, &m_colonMesh.vBuffer, &stride, &offset);
        ctx->Draw(m_colonMesh.vertexCount, 0);
    }
};
