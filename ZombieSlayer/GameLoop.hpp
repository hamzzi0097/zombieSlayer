#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"
#include "Logger.hpp"
#include "Collider.hpp"
#include "PlayerBulletSpawner.hpp"
#include "PlayerControl.hpp"
#include "MonsterSpawner.hpp"
#include "MeshRenderer.hpp"
#include "HeartUI.hpp"
#include "HitEffect.hpp"
#include "BombCooldownUI.hpp"
#include "PlayerHealth.hpp"
#include "BombSpawner.hpp"

// 원 그리기
std::vector<Vertex> CreateCircleVertices(float radius, int segments, XMFLOAT4 color)
{
    std::vector<Vertex> vertices;

    for (int i = 0; i < segments; i++)
    {
        float angle0 = XM_2PI * i / segments;
        float angle1 = XM_2PI * (i + 1) / segments;

        vertices.push_back({ { 0.0f, 0.0f, 0.0f }, color });
        vertices.push_back({ { cosf(angle1) * radius, sinf(angle1) * radius, 0.0f }, color });
        vertices.push_back({ { cosf(angle0) * radius, sinf(angle0) * radius, 0.0f }, color });
    }

    return vertices;
}

class GameLoop {
public:
    enum class State { Lobby, Playing, GameOver };

    WindowContext win;
    DeltaTime timer;
    std::vector<GameObject*> world;
    std::vector<GameObject*> pendingObjects;

    ShaderSet shader;

    // 게임 내내 재사용하는 리소스 (Initialize에서 1회 생성)
    Mesh*          playerMesh     = nullptr;
    Mesh*          bombMesh       = nullptr;
    ColorMaterial* playerMaterial = nullptr;
    ColorMaterial* playerBulletMaterial = nullptr;  // 탄환 전용 ColorMaterial

    // UI 루트 GameObject (= 상태별 캔버스). Initialize에서 1회 생성, 소멸자에서 1회 정리.
    // 패널은 별도 클래스 없이 "끌 수 있는 GameObject"로 두고, 상태 루프가 해당 UI만 구동한다.
    GameObject* playingUI  = nullptr;
    GameObject* gameOverUI = nullptr;
    GameObject* lobbyUI    = nullptr;
    HitEffect*  hitEffect  = nullptr;   // 라운드마다 onDamaged 콜백 재바인딩용 핸들

    // Playing 진입 시 생성, GameOver 퇴장 시 world와 함께 삭제
    GameObject* player          = nullptr;
    GameObject* monsterSpawner  = nullptr;

    GameLoop() {
        LOG_DEBUG("GameLoop Created.");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        for (auto obj : pendingObjects) delete obj;
        pendingObjects.clear();
        delete playingUI;      playingUI      = nullptr;
        delete gameOverUI;     gameOverUI     = nullptr;
        delete lobbyUI;        lobbyUI        = nullptr;
        delete playerMesh;     playerMesh     = nullptr;
        delete playerMaterial; playerMaterial = nullptr;
        delete playerBulletMaterial; playerBulletMaterial = nullptr;
        delete bombMesh;       bombMesh       = nullptr;
        GraphicsContext::Destroy();
        shader.Release();
        LOG_DEBUG("GameLoop Destroyed.");
    }

    /// <summary>
    /// 클래스 초기화 함수, main문 시작 시 1회만 호출
    /// </summary>
    bool Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
                    int w = 800, int h = 600) {
        if (!win.Initialize(hInst, w, h, wndProc)) return false;

        if (!GraphicsContext::Create(win.hWnd, w, h)) return false;

        D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // 셰이더 컴파일 및 생성
        shader = GraphicsContext::Get()->CompileAndCreate(L"effect.hlsl", 0, true, ied, 2);

        // 게임 내내 재사용하는 메시/머테리얼 리소스 1회 생성
        std::vector<Vertex> playerVertices =
            CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

        playerMesh = new Mesh();
        playerMesh->Create(playerVertices);

        std::vector<Vertex> bombVertices =
            CreateCircleVertices(1.0f, 64, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

        bombMesh = new Mesh();
        bombMesh->Create(bombVertices);

        playerMaterial = new ColorMaterial(shader, XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f));
        playerBulletMaterial = new ColorMaterial(shader, XMFLOAT4(1.0f, 0.9f, 0.2f, 1.0f));

        // ── UI 캔버스 1회 구성 ──────────────────────────────────────────────
        // 패널 = 끌 수 있는 GameObject. 요소 = Component. player는 이중 포인터(&player)로
        // 넘겨 라운드마다 재생성돼도 항상 현재 플레이어를 가리키게 한다.
        hitEffect = new HitEffect();

        playingUI = new GameObject(0.0f, 0.0f, 0.0f);
        playingUI->AddComponent(new HeartUI(shader, &player));
        playingUI->AddComponent(hitEffect);
        playingUI->AddComponent(new BombCooldownUI(shader, &player));

        // GameOver 캔버스: player==nullptr이라 빈 하트로 표시됨 (결과 UI는 추후 추가)
        gameOverUI = new GameObject(0.0f, 0.0f, 0.0f);
        gameOverUI->AddComponent(new HeartUI(shader, &player));

        // Lobby 캔버스: 타이틀 UI는 추후 추가
        lobbyUI = new GameObject(0.0f, 0.0f, 0.0f);

        return true;
    }

    /// <summary>
    /// state 전환 + 각 state 전환 전후에 처리될 로직 
    /// </summary>
    /// <param name="next"></param>
    void ChangeState(State next) {
        OnExit(m_state);
        m_state = next;
        OnEnter(m_state);
    }

    void Run() {
        LOG_DEBUG("Game loop started");
        OnEnter(m_state);
        MSG msg = {};
        while (msg.message != WM_QUIT && m_isRunning) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                float dt = timer.GetDelta();
                Input();
                Update(dt);
                Render();
            }
        }
        LOG_DEBUG("Game loop ended");
    }

private:
    State m_state     = State::Lobby;
    bool  m_isRunning = true;

    // Collider가 붙은 모든 오브젝트 쌍 체크 & 충돌 이벤트 전달
    void CheckOnCollisions()
    {
        for (size_t i = 0; i < world.size(); i++)
        {
            Collider* curCollider_1 = world[i]->GetComponent<Collider>();
            if (!curCollider_1) continue;

            for (size_t j = i + 1; j < world.size(); j++)
            {
                if (world[i]->isObjDead || world[j]->isObjDead) continue;

                Collider* curCollider_2 = world[j]->GetComponent<Collider>();
                if (!curCollider_2) continue;

                if (curCollider_1->CheckCollision(curCollider_2))
                {
                    world[i]->OnCollision(world[j]);
                    world[j]->OnCollision(world[i]);
                }
            }
        }
    }

    /// <summary>
    /// 각 상태별 오브젝트 생성
    /// </summary>
    /// <param name="전환 이후 상태"></param>
    void OnEnter(State s) {
        switch (s) {
        case State::Lobby:
            LOG_DEBUG("State Enter: Lobby");
            LOG_INFO("=== ZombieSlayer ===");
            LOG_INFO("Press SPACE to start / ESC to end game");


            break;
        case State::Playing:
        {
            LOG_DEBUG("State Enter: Playing");
            LOG_INFO("Game Start! Survive as long as you can.");

            // 플레이어 오브젝트 생성
            player = new GameObject(0.0f, 0.0f, 0.0f);
            player->scale = { 0.08f, 0.08f, 1.0f };
            player->AddComponent(new MeshRenderer(playerMesh, playerMaterial));
            player->AddComponent(new PlayerController(win.hWnd, &win.Width, &win.Height));
            player->AddComponent(new PlayerHealth(3, 1.5f));
            player->AddComponent(new PlayerBulletSpawner(
                &pendingObjects, playerMesh, playerBulletMaterial,
                win.hWnd, &win.Width, &win.Height
            ));
            player->AddComponent(new BombSpawner(&world, &pendingObjects, bombMesh, shader));
            player->AddComponent(new CircleCollider(1.0f, CollisionLayer::Player));

            // 몬스터 스포너 오브젝트 생성
            std::vector<Vertex> monsterVertices =
                CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
            monsterSpawner = new GameObject(100, 100, 100);
            monsterSpawner->AddComponent(new MonsterSpawner(&pendingObjects, player, shader, monsterVertices));

            world.push_back(player);
            world.push_back(monsterSpawner);

            // UI는 Initialize에서 이미 구성됨. 새 player의 Health 콜백만 재바인딩.
            PlayerHealth* health = player->GetComponent<PlayerHealth>();
            if (health)
            {
                health->onDamaged = [this]() { hitEffect->Trigger(); };
            }

            break;
        }
        case State::GameOver:
            LOG_DEBUG("State Enter: GameOver");
            LOG_INFO("Game Over! Press SPACE to restart / 1 to lobby");

            // 점수 및 ui 오브젝트 추가
            break;
        }
    }

    /// <summary>
    /// 각 상태별 오브젝트 삭제
    /// </summary>
    /// <param name="전환 이전 상태"></param>
    void OnExit(State s) {
        switch (s) {
        case State::Lobby:    LOG_DEBUG("State Exit: Lobby");    break;
        case State::Playing:
            LOG_DEBUG("State Exit: Playing");
            // Playing 종료 시 대기 중인 pendingObjects만 정리 (uiManager는 GameOver에서도 유지)
            for (auto obj : pendingObjects) delete obj;
            pendingObjects.clear();
            break;
        case State::GameOver:
            LOG_DEBUG("State Exit: GameOver");
            // GameOver 종료 시 world만 제거 (UI 캔버스는 영속, 소멸자에서 정리)
            for (auto obj : world) delete obj;
            world.clear();
            player         = nullptr;   // world에서 이미 delete됨
            monsterSpawner = nullptr;
            break;
        }
    }

    void Input() {
        switch (m_state) {
        case State::Lobby:
            if (GetAsyncKeyState(VK_SPACE) & 0x0001)
                ChangeState(State::Playing);

            if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
                m_isRunning = false;

            GetAsyncKeyState('1');  // 쓸모없는 입력 비트 소비
                
            break;
        case State::Playing:
            for (auto obj : world) obj->Input();

            GetAsyncKeyState(VK_ESCAPE);
            GetAsyncKeyState(VK_SPACE);
            GetAsyncKeyState('1');

            break;
        case State::GameOver:
            if (GetAsyncKeyState(VK_SPACE) & 0x0001)
                ChangeState(State::Playing);

            GetAsyncKeyState(VK_ESCAPE);    // 쓸모없는 입력 비트 소비

            if (GetAsyncKeyState('1') & 0x0001)
                ChangeState(State::Lobby);
            break;
        }
    }

    void Update(float dt) {
        switch (m_state) {
        case State::Lobby:
            lobbyUI->Update(dt);
            break;
        case State::Playing:
            // 생성 예약된 오브젝트를 world로 push_back
            for (auto obj : pendingObjects) world.push_back(obj);
            pendingObjects.clear();

            for (auto obj : world) obj->Update(dt);
            playingUI->Update(dt);

            // 이동 업데이트 이후 죽은 오브젝트 제거 전 충돌 검사
            CheckOnCollisions();

            // 플레이어 사망 → GameOver 전환 (dead 오브젝트 제거 루프보다 먼저 체크)
            if (player && player->isObjDead) {
                ChangeState(State::GameOver);
                break;
            }

            // 죽음 표시된 오브젝트를 world에서 제거
            for (auto obj = world.begin(); obj != world.end(); ) {
                if ((*obj)->isObjDead) {
                    delete* obj;
                    obj = world.erase(obj);
                    continue;
                }
                obj++;

                continue;
            }
            break;
        case State::GameOver:
            gameOverUI->Update(dt);
            break;
        }
    }

    void Render() {
        auto* gfx = GraphicsContext::Get();
        switch (m_state) {
        case State::Lobby:

            break;
        case State::Playing: 
        {
            float col[] = { 0.1f, 0.2f, 0.3f, 1.0f };
            gfx->ImmediateContext->ClearRenderTargetView(gfx->RTV, col);

            D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
            gfx->ImmediateContext->RSSetViewports(1, &vp);
            gfx->ImmediateContext->OMSetRenderTargets(1, &gfx->RTV, nullptr);
            gfx->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            float blendFactor[4] = { 0, 0, 0, 0 };
            gfx->ImmediateContext->OMSetBlendState(gfx->AlphaBlendState, blendFactor, 0xffffffff);

            for (auto obj : world) obj->Render();
            playingUI->Render();  // UI는 항상 게임 오브젝트 위에 렌더링
            break;
        }
        case State::GameOver:
        {
            float col[] = { 0.05f, 0.05f, 0.05f, 1.0f };   // 어두운 배경
            gfx->ImmediateContext->ClearRenderTargetView(gfx->RTV, col);

            D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
            gfx->ImmediateContext->RSSetViewports(1, &vp);
            gfx->ImmediateContext->OMSetRenderTargets(1, &gfx->RTV, nullptr);
            gfx->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            gameOverUI->Render();  // player==null → 빈 하트 (0개 상태로 표시)
            break;
        }
        }
        gfx->SwapChain->Present(gfx->VSync, 0);
    }
};
