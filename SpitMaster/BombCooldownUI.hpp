#pragma once
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "BombSpawner.hpp"

// [BombCooldownUI 컴포넌트]
// 플레이어의 BombSpawner를 읽어 화면 오른쪽 하단에 폭탄 쿨다운을 파이 게이지로 표시.
// 회색 배경 원 위에, 진행도(0~1)만큼 12시 방향부터 시계방향으로 노란 파이가 채워진다.
//
// player를 이중 포인터(GameObject**)로 받아, 플레이어가 매 라운드 재생성돼도
// 항상 현재 플레이어의 BombSpawner를 읽는다. player==nullptr이면 아무것도 안 그린다.
//
// 메시 전략: 풀 원형 삼각형 팬(N세그먼트)을 12시 시작·시계방향으로 한 번만 생성.
// [center, p_i, p_(i+1)] 순서라, 앞쪽 k세그먼트(=k*3 정점)만 Draw하면 부분 파이가 된다.
// 매 프레임 버퍼를 다시 만들지 않고 Draw 정점 수만 바꾼다.
class BombCooldownUI : public Component {
public:
    BombCooldownUI(ShaderSet shaders, GameObject** playerPtr)
        : m_shaders(shaders), m_playerPtr(playerPtr) {}

    ~BombCooldownUI() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_matFill;
        delete m_matBack;
    }

    void Start() override {
        m_pieMesh.Create(BuildPieVertices());

        m_matBack = new ColorMaterial(m_shaders, XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f)); // 배경 회색
        m_matFill = new ColorMaterial(m_shaders, XMFLOAT4(1.00f, 0.80f, 0.10f, 1.0f)); // 폭탄 노랑

        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage     = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_cBuffer) return;

        // 현재 플레이어의 BombSpawner에서 쿨다운 진행도 읽기
        GameObject* player = m_playerPtr ? *m_playerPtr : nullptr;
        if (!player) return;

        BombSpawner* spawner = player->GetComponent<BombSpawner>();
        if (!spawner) return;

        float progress = spawner->GetCooldownRatio();
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        GraphicsContext* gfx = GraphicsContext::Get();
        const float aspect  = gfx->GetAspectRatio();
        const float size    = 0.10f;   // 게이지 반지름 (NDC)
        const float baseX   =  0.82f;   // 오른쪽 하단 (HeartUI 좌하단과 대칭)
        const float baseY   = -0.82f;

        float sx = (aspect >= 1.0f) ? size / aspect : size;
        float sy = (aspect >= 1.0f) ? size           : size * aspect;

        XMMATRIX world = XMMatrixScaling(sx, sy, 1.0f) *
                         XMMatrixTranslation(baseX, baseY, 0.0f);

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(world);
        gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

        UINT stride = sizeof(Vertex), offset = 0;
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_pieMesh.vBuffer, &stride, &offset);

        // 1) 배경 원 전체를 회색으로
        m_matBack->Bind();
        gfx->ImmediateContext->Draw(m_pieMesh.vertexCount, 0);

        // 2) 진행도만큼 앞쪽 세그먼트만 노랑으로 덮어 그림
        int fillSegments = (int)(progress * SEGMENTS + 0.5f);
        if (fillSegments > 0) {
            m_matFill->Bind();
            gfx->ImmediateContext->Draw((UINT)(fillSegments * 3), 0);
        }
    }

    void Input()          override {}
    void Update(float dt) override {}

private:
    static constexpr int SEGMENTS = 48;

    ShaderSet      m_shaders;
    GameObject**   m_playerPtr = nullptr;
    Mesh           m_pieMesh;
    ColorMaterial* m_matBack   = nullptr;
    ColorMaterial* m_matFill   = nullptr;
    ID3D11Buffer*  m_cBuffer   = nullptr;

    // 12시(상단)에서 시작해 시계방향으로 도는 풀 원형 삼각형 팬.
    // 세그먼트 i = [center, p_i, p_(i+1)] 순서라 앞쪽 k개만 그리면 부분 파이.
    std::vector<Vertex> BuildPieVertices() {
        constexpr float PI = 3.14159265358979f;

        std::vector<XMFLOAT2> pts(SEGMENTS + 1);
        for (int i = 0; i <= SEGMENTS; i++) {
            float a = PI * 0.5f - 2.0f * PI * i / SEGMENTS;   // 상단 시작, 시계방향
            pts[i] = { cosf(a), sinf(a) };
        }

        Vertex center = { {0.0f, 0.0f, 0.0f}, {1,1,1,1} };
        std::vector<Vertex> verts;
        verts.reserve(SEGMENTS * 3);
        for (int i = 0; i < SEGMENTS; i++) {
            verts.push_back(center);
            verts.push_back({ {pts[i].x,     pts[i].y,     0.0f}, {1,1,1,1} });
            verts.push_back({ {pts[i + 1].x, pts[i + 1].y, 0.0f}, {1,1,1,1} });
        }
        return verts;
    }
};
