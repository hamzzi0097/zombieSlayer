#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// [TextUI 컴포넌트]
// 5x7 블록 폰트로 문자열을 화면(NDC)에 렌더링하는 재사용 텍스트 요소.
// 타이틀("ZOMBIE SLAYER"), "GAME OVER", 안내 문구 등 정적 라벨에 사용.
//
// 동작 개요
//  - 글자 1개 = 5(가로) x 7(세로) 픽셀 격자. 켜진 픽셀마다 사각형 1개 생성.
//  - Start()에서 문자열 전체를 "텍스트 공간"(가로 단위, 세로 7단위) 하나의 메시로 베이크.
//    → 매 프레임 Draw 1회. 글자별 행렬 계산 불필요.
//  - Render()에서 월드 행렬로 크기/위치/화면비를 입혀 NDC에 배치.
//    텍스트는 (centerX, centerY)를 '중심'으로 가로/세로 가운데 정렬된다.
//
// 좌표 규칙(기존 UI와 동일)
//  - 정점은 시계방향(CW) 와인딩 → DX11 기본 백페이스 컬링에서 살아남는다.
//  - effect.hlsl(POSITION+COLOR) + ColorMaterial 사용. 월드 행렬은 b0.
class TextUI : public Component {
public:
    // 가로 정렬 기준. anchor(centerX)를 기준으로 텍스트를 어디에 붙일지 결정.
    //  Left   : centerX에서 오른쪽으로 뻗음 (좌측 고정)
    //  Center : centerX를 가운데로 (기본)
    //  Right  : centerX에서 왼쪽으로 뻗음 (우측 고정 — 자릿수 변해도 끝이 안 흔들림)
    enum class Align { Left, Center, Right };

    // text     : 표시할 문자열 (소문자는 자동 대문자화. 폰트는 대문자/숫자/일부 기호만)
    // centerX/Y: 정렬 기준 NDC 좌표 (세로는 항상 가운데)
    // height   : 글자 높이(NDC). 가로폭은 화면비로 자동 보정되어 정사각 픽셀 유지
    // color    : 글자 색
    // align    : 가로 정렬 (기본 Center)
    TextUI(ShaderSet shaders, const std::string& text,
           float centerX, float centerY, float height, XMFLOAT4 color,
           Align align = Align::Center)
        : m_shaders(shaders), m_text(text),
          m_centerX(centerX), m_centerY(centerY), m_height(height),
          m_color(color), m_align(align) {}

    ~TextUI() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_material;
    }

    void Start() override {
        m_mesh.Create(BuildTextVertices(m_text));

        m_material = new ColorMaterial(m_shaders, m_color);

        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage     = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_cBuffer || !m_mesh.vBuffer) return;

        GraphicsContext* gfx = GraphicsContext::Get();
        const float aspect = gfx->GetAspectRatio();

        // 7단위 격자 높이를 NDC height에 맞춤. 가로는 화면비로 나눠 정사각 픽셀 유지.
        const float sy = m_height / 7.0f;
        const float sx = (aspect >= 1.0f) ? sy / aspect : sy;
        const float syFinal = (aspect >= 1.0f) ? sy : sy * aspect;

        XMMATRIX world = XMMatrixScaling(sx, syFinal, 1.0f) *
                         XMMatrixTranslation(m_centerX, m_centerY, 0.0f);

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(world);
        gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

        m_material->Bind();

        UINT stride = sizeof(Vertex), offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_mesh.vBuffer, &stride, &offset);
        gfx->ImmediateContext->Draw(m_mesh.vertexCount, 0);
    }

    void Input()          override {}
    void Update(float dt) override {}

    // 색 변경(다음 Render부터 반영)
    void SetColor(XMFLOAT4 c) { m_color = c; if (m_material) m_material->SetColor(c); }

    // 동적 텍스트 변경. 내용이 실제로 바뀐 경우에만 메시를 재생성한다.
    // Start() 이전 호출이면 문자열만 저장하고, Start()에서 메시가 만들어진다.
    void SetText(const std::string& text) {
        if (text == m_text) return;
        m_text = text;
        if (m_cBuffer) { // Start() 이미 끝남(=GPU 리소스 준비됨) → 즉시 재생성
            if (m_mesh.vBuffer) { m_mesh.vBuffer->Release(); m_mesh.vBuffer = nullptr; }
            m_mesh.Create(BuildTextVertices(m_text));
        }
    }

private:
    ShaderSet      m_shaders;
    std::string    m_text;
    float          m_centerX, m_centerY, m_height;
    XMFLOAT4       m_color;
    Align          m_align = Align::Center;

    Mesh           m_mesh;
    ColorMaterial* m_material = nullptr;
    ID3D11Buffer*  m_cBuffer  = nullptr;

    // ── 글자 레이아웃 상수 ────────────────────────────────────────────────
    static constexpr int   GLYPH_W = 5;   // 글자 가로 픽셀
    static constexpr int   GLYPH_H = 7;   // 글자 세로 픽셀
    static constexpr float ADVANCE = 6.0f; // 다음 글자까지 가로 이동(=5 + 1칸 간격)

    // 직사각형 → 삼각형 2개. 시계방향(CW) 와인딩(프로젝트 공통 백페이스 컬링 규칙).
    static void PushRect(std::vector<Vertex>& v, float x0, float y0, float x1, float y1) {
        XMFLOAT4 w = { 1,1,1,1 };
        v.push_back({ {x0,y0,0}, w }); v.push_back({ {x1,y1,0}, w }); v.push_back({ {x1,y0,0}, w });
        v.push_back({ {x0,y0,0}, w }); v.push_back({ {x0,y1,0}, w }); v.push_back({ {x1,y1,0}, w });
    }

    // 문자열을 텍스트 공간 메시로 베이크. 가로/세로 모두 원점 중심 정렬.
    std::vector<Vertex> BuildTextVertices(const std::string& text) {
        std::vector<Vertex> v;
        const int n = (int)text.size();
        if (n == 0) { PushDegenerate(v); return v; }

        const float totalW = n * ADVANCE - 1.0f; // 마지막 글자 뒤 간격 제외
        float offX;
        if      (m_align == Align::Left)  offX = 0.0f;       // 좌측 고정
        else if (m_align == Align::Right) offX = -totalW;    // 우측 고정
        else                              offX = -totalW * 0.5f; // 가운데
        const float offY = -GLYPH_H * 0.5f;       // 세로 가운데(7/2)

        for (int gi = 0; gi < n; ++gi) {
            const uint8_t* rows = FindRows(ToUpper(text[gi]));
            const float penX = offX + gi * ADVANCE;
            if (!rows) continue; // 공백/미지원 문자 → 자리만 차지

            for (int r = 0; r < GLYPH_H; ++r) {
                uint8_t row = rows[r];
                for (int c = 0; c < GLYPH_W; ++c) {
                    // bit4 = 가장 왼쪽 열(c=0)
                    if (row & (1 << (GLYPH_W - 1 - c))) {
                        float x0 = penX + c;
                        float x1 = x0 + 1.0f;
                        float y1 = (GLYPH_H - r)     + offY; // 위(행 0이 가장 높음)
                        float y0 = (GLYPH_H - r - 1) + offY; // 아래
                        PushRect(v, x0, y0, x1, y1);
                    }
                }
            }
        }
        if (v.empty()) PushDegenerate(v);
        return v;
    }

    static void PushDegenerate(std::vector<Vertex>& v) {
        XMFLOAT4 z = {};
        v.push_back({ {0,0,0}, z }); v.push_back({ {0,0,0}, z }); v.push_back({ {0,0,0}, z });
    }

    static char ToUpper(char c) {
        return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
    }

    // ── 5x7 폰트 테이블 ───────────────────────────────────────────────────
    // 각 글자 = 7행. 행마다 하위 5비트가 좌→우 픽셀(bit4=왼쪽).
    // 지원: A-Z, 0-9, 공백, ':', '!', '.', '-'
    static const uint8_t* FindRows(char c) {
        struct G { char c; uint8_t r[7]; };
        static const G T[] = {
            {'A', {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
            {'B', {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110}},
            {'C', {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110}},
            {'D', {0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110}},
            {'E', {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111}},
            {'F', {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000}},
            {'G', {0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01110}},
            {'H', {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001}},
            {'I', {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b11111}},
            {'J', {0b00111,0b00010,0b00010,0b00010,0b10010,0b10010,0b01100}},
            {'K', {0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001}},
            {'L', {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111}},
            {'M', {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001}},
            {'N', {0b10001,0b10001,0b11001,0b10101,0b10011,0b10001,0b10001}},
            {'O', {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}},
            {'P', {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000}},
            {'Q', {0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101}},
            {'R', {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001}},
            {'S', {0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110}},
            {'T', {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100}},
            {'U', {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110}},
            {'V', {0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100}},
            {'W', {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001}},
            {'X', {0b10001,0b10001,0b01010,0b00100,0b01010,0b10001,0b10001}},
            {'Y', {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100}},
            {'Z', {0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111}},
            {'0', {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}},
            {'1', {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b11111}},
            {'2', {0b01110,0b10001,0b00001,0b00110,0b01000,0b10000,0b11111}},
            {'3', {0b11111,0b00010,0b00100,0b00010,0b00001,0b10001,0b01110}},
            {'4', {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}},
            {'5', {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}},
            {'6', {0b01110,0b10000,0b10000,0b11110,0b10001,0b10001,0b01110}},
            {'7', {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}},
            {'8', {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}},
            {'9', {0b01110,0b10001,0b10001,0b01111,0b00001,0b00001,0b01110}},
            {':', {0b00000,0b00100,0b00000,0b00000,0b00000,0b00100,0b00000}},
            {'!', {0b00100,0b00100,0b00100,0b00100,0b00100,0b00000,0b00100}},
            {'.', {0b00000,0b00000,0b00000,0b00000,0b00000,0b00000,0b00100}},
            {'-', {0b00000,0b00000,0b00000,0b01110,0b00000,0b00000,0b00000}},
        };
        for (const G& g : T)
            if (g.c == c) return g.r;
        return nullptr; // 공백 포함 미지원 문자
    }
};
