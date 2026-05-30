#pragma once
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// Health.hpp가 존재하는 브랜치에서는 아래 include를 활성화
// #include "Health.hpp"

// [HeartUI 컴포넌트]
// 플레이어의 Health 컴포넌트를 읽어 화면 왼쪽 하단에 하트 UI를 렌더링.
// UIManager가 소유하는 별도 GameObject에 부착해 플레이어와 생명주기를 분리한다.
// 하트 메시: 파라메트릭 방정식 x=16sin³t, y=13cos t-5cos 2t-2cos 3t-cos 4t
// 색상: 남은 목숨 > 1 → 빨강 / 목숨 1 → 주황(경고) / 빈 하트 → 어두운 회색
class HeartUI : public Component {
public:
    HeartUI(ShaderSet shaders, GameObject* player, int maxLives = 3)
        : m_shaders(shaders), m_player(player), m_maxLives(maxLives) {}

    ~HeartUI() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_matFull;
        delete m_matLow;
        delete m_matEmpty;
    }

    void Start() override {
        m_heartMesh.Create(BuildHeartVertices());

        m_matFull  = new ColorMaterial(m_shaders, XMFLOAT4(0.85f, 0.06f, 0.06f, 1.0f)); // 빨강
        m_matLow   = new ColorMaterial(m_shaders, XMFLOAT4(1.00f, 0.40f, 0.00f, 1.0f)); // 주황
        m_matEmpty = new ColorMaterial(m_shaders, XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f)); // 회색

        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage     = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_cBuffer) return;

        // Health 컴포넌트가 있으면 실제 목숨 수 반영, 없으면 최대치로 표시
        int lives = m_maxLives;

// Health.hpp가 존재하는 브랜치에서는 아래 블록을 활성화
/*
        if (m_player) {
            Health* hp = m_player->GetComponent<Health>();
            if (hp) {
                lives      = hp->GetLives();
                m_maxLives = hp->GetMaxLives();
            }
        }
*/

        GraphicsContext* gfx = GraphicsContext::Get();
        const float aspect  = gfx->GetAspectRatio();
        const float size    = 0.10f;   // 하트 기준 크기 (NDC)
        const float spacing = 0.22f;   // 하트 간격
        const float baseX   = -0.90f;  // 첫 번째 하트 x
        const float baseY   = -0.82f;  // 하트 y (화면 하단)

        float sx = (aspect >= 1.0f) ? size / aspect : size;
        float sy = (aspect >= 1.0f) ? size           : size * aspect;

        for (int i = 0; i < m_maxLives; i++) {
            ColorMaterial* mat = nullptr;
            if (i < lives)
                mat = (lives == 1) ? m_matLow : m_matFull;
            else
                mat = m_matEmpty;

            mat->Bind();

            float posX = baseX + i * spacing;
            XMMATRIX world = XMMatrixScaling(sx, sy, 1.0f) *
                             XMMatrixTranslation(posX, baseY, 0.0f);

            ConstantBuffer cb;
            cb.matWorld = XMMatrixTranspose(world);
            gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
            gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

            UINT stride = sizeof(Vertex), offset = 0;
            gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_heartMesh.vBuffer, &stride, &offset);
            gfx->ImmediateContext->Draw(m_heartMesh.vertexCount, 0);
        }
    }

    void Input()           override {}
    void Update(float dt)  override {}

private:
    ShaderSet      m_shaders;
    GameObject*    m_player   = nullptr;
    int            m_maxLives = 3;
    Mesh           m_heartMesh;
    ColorMaterial* m_matFull  = nullptr;
    ColorMaterial* m_matLow   = nullptr;
    ColorMaterial* m_matEmpty = nullptr;
    ID3D11Buffer*  m_cBuffer  = nullptr;

    // 파라메트릭 하트 방정식으로 삼각형 팬 메시 생성
    // x = 16sin³(t),  y = 13cos(t) - 5cos(2t) - 2cos(3t) - cos(4t)
    // 정규화: /17.0f,  y 오프셋 +6/17 으로 시각 중심을 (0,0)에 정렬
    std::vector<Vertex> BuildHeartVertices() {
        constexpr int   N     = 64;
        constexpr float PI    = 3.14159265358979f;
        constexpr float SCALE = 1.0f / 17.0f;
        constexpr float Y_OFF = 6.0f / 17.0f;

        std::vector<XMFLOAT2> pts(N);
        for (int i = 0; i < N; i++) {
            float t = 2.0f * PI * i / N;
            float s = sinf(t);
            float x =  16.0f * s * s * s;
            float y =  13.0f * cosf(t)
                     -  5.0f * cosf(2.0f * t)
                     -  2.0f * cosf(3.0f * t)
                     -         cosf(4.0f * t);
            pts[i] = { x * SCALE, y * SCALE + Y_OFF };
        }

        Vertex center = { {0.0f, 0.0f, 0.0f}, {1,1,1,1} };
        std::vector<Vertex> verts;
        verts.reserve(N * 3);
        for (int i = 0; i < N; i++) {
            int j = (i + 1) % N;
            verts.push_back(center);
            verts.push_back({ {pts[i].x, pts[i].y, 0.0f}, {1,1,1,1} });
            verts.push_back({ {pts[j].x, pts[j].y, 0.0f}, {1,1,1,1} });
        }
        return verts;
    }
};
