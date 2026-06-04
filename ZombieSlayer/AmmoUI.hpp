#pragma once
#include "ObjectBase.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "PlayerBulletSpawner.hpp"

// [AmmoUI]
// 화면 좌하단에 탄창 슬롯(10칸)과 재장전 진행 바를 렌더링한다.
// - 탄창: 남은 탄 수만큼 노란 사각형, 소진된 슬롯은 회색.
// - 재장전 바: 재장전 중일 때만 표시, 좌→우로 채워진다.
// player를 이중 포인터(GameObject**)로 받아 라운드마다 재생성돼도 현재 플레이어를 참조한다.
class AmmoUI : public Component {
public:
    AmmoUI(ShaderSet shaders, GameObject** playerPtr)
        : m_shaders(shaders), m_playerPtr(playerPtr) {}

    ~AmmoUI() {
        if (m_cBuffer) m_cBuffer->Release();
        delete m_matFill;
        delete m_matEmpty;
        delete m_matBar;
        delete m_matBarBg;
    }

    void Start() override {
        m_slotMesh.Create(BuildSlotVertices());
        m_barMesh.Create(BuildBarVertices());

        m_matFill  = new ColorMaterial(m_shaders, XMFLOAT4(1.0f, 0.9f, 0.2f, 1.0f));   // 탄 노랑
        m_matEmpty = new ColorMaterial(m_shaders, XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));   // 빈 슬롯 회색
        m_matBar   = new ColorMaterial(m_shaders, XMFLOAT4(0.4f, 0.9f, 1.0f, 1.0f));   // 재장전 바 시안
        m_matBarBg = new ColorMaterial(m_shaders, XMFLOAT4(0.12f, 0.12f, 0.12f, 1.0f)); // 바 배경

        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage     = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ConstantBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        GraphicsContext::Get()->Device->CreateBuffer(&cbd, nullptr, &m_cBuffer);
    }

    void Render() override {
        if (!m_cBuffer) return;

        GameObject* player = m_playerPtr ? *m_playerPtr : nullptr;
        if (!player) return;

        PlayerBulletSpawner* spawner = player->GetComponent<PlayerBulletSpawner>();
        if (!spawner) return;

        int   currentAmmo    = spawner->GetCurrentAmmo();
        bool  reloading      = spawner->IsReloading();
        float reloadProgress = spawner->GetReloadProgress(); // 0.0→1.0

        GraphicsContext* gfx = GraphicsContext::Get();

        ConstantBuffer cb;
        cb.matWorld = XMMatrixTranspose(XMMatrixIdentity());
        gfx->ImmediateContext->UpdateSubresource(m_cBuffer, 0, nullptr, &cb, 0, 0);
        gfx->ImmediateContext->VSSetConstantBuffers(0, 1, &m_cBuffer);

        UINT stride = sizeof(Vertex), offset = 0;

        // ── 탄창 슬롯 ──────────────────────────────────────────────────
        gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_slotMesh.vBuffer, &stride, &offset);

        // 전체 슬롯 회색 배경 
        m_matEmpty->Bind();
        gfx->ImmediateContext->Draw(m_slotMesh.vertexCount, 0);

        // 남은 탄 수만큼 노랑 덮기 (재장전 중이면 표시 안 함)
        if (!reloading && currentAmmo > 0) {
            m_matFill->Bind();
            gfx->ImmediateContext->Draw((UINT)(currentAmmo * VERTS_PER_SLOT), 0);
        }

        // ── 재장전 바 ──────────────────────────────────────────────────
        if (reloading) {
            gfx->ImmediateContext->IASetVertexBuffers(0, 1, &m_barMesh.vBuffer, &stride, &offset);

            m_matBarBg->Bind();
            gfx->ImmediateContext->Draw(m_barMesh.vertexCount, 0);

            int fillSegs = (int)(reloadProgress * BAR_SEGS + 0.5f);
            if (fillSegs > 0) {
                m_matBar->Bind();
                gfx->ImmediateContext->Draw((UINT)(fillSegs * VERTS_PER_SLOT), 0);
            }
        }
    }

    void Input()          override {}
    void Update(float dt) override {}

private:
    static constexpr int   MAX_AMMO       = 10;
    static constexpr int   BAR_SEGS       = 20;
    static constexpr int   VERTS_PER_SLOT = 6;   // 사각형 = 삼각형 2개 = 정점 6개

    // 슬롯 레이아웃 (NDC 좌표, 좌하단)
    static constexpr float SLOT_X0  = 0.3f;  // 슬롯 영역 왼쪽 끝
    static constexpr float SLOT_Y1  = -0.78f;  // 슬롯 위
    static constexpr float SLOT_Y0  = -0.86f;  // 슬롯 아래
    static constexpr float SLOT_W   = 0.033f;  // 슬롯 하나 너비
    static constexpr float SLOT_GAP = 0.007f;  // 슬롯 간 간격

    // 재장전 바 레이아웃
    static constexpr float BAR_Y1 = -0.88f;
    static constexpr float BAR_Y0 = -0.92f;

    ShaderSet      m_shaders;
    GameObject**   m_playerPtr = nullptr;
    Mesh           m_slotMesh;
    Mesh           m_barMesh;
    ColorMaterial* m_matFill   = nullptr;
    ColorMaterial* m_matEmpty  = nullptr;
    ColorMaterial* m_matBar    = nullptr;
    ColorMaterial* m_matBarBg  = nullptr;
    ID3D11Buffer*  m_cBuffer   = nullptr;

    // CW 와인딩: (x0,y0)=좌하, (x1,y1)=우상
    static void PushRect(std::vector<Vertex>& v, float x0, float y0, float x1, float y1) {
        XMFLOAT4 w = { 1,1,1,1 };
        v.push_back({ {x0,y0,0}, w }); v.push_back({ {x1,y1,0}, w }); v.push_back({ {x1,y0,0}, w });    // ◢
        v.push_back({ {x0,y0,0}, w }); v.push_back({ {x0,y1,0}, w }); v.push_back({ {x1,y1,0}, w });    // ◤
    }

    std::vector<Vertex> BuildSlotVertices() {
        std::vector<Vertex> verts;
        verts.reserve(MAX_AMMO * VERTS_PER_SLOT);
        for (int i = 0; i < MAX_AMMO; i++) {
            float x0 = SLOT_X0 + i * (SLOT_W + SLOT_GAP);
            PushRect(verts, x0, SLOT_Y0, x0 + SLOT_W, SLOT_Y1);
        }
        return verts;
    }

    std::vector<Vertex> BuildBarVertices() {
        float totalW = MAX_AMMO * (SLOT_W + SLOT_GAP) - SLOT_GAP;
        float segW   = totalW / BAR_SEGS;
        std::vector<Vertex> verts;
        verts.reserve(BAR_SEGS * VERTS_PER_SLOT);
        for (int i = 0; i < BAR_SEGS; i++) {
            float x0 = SLOT_X0 + i * segW;
            PushRect(verts, x0, BAR_Y0, x0 + segW - 0.002f, BAR_Y1);
        }
        return verts;
    }
};
