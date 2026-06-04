#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"
#include "Logger.hpp"
#include "Collider.hpp"
#include "PlayerBulletSpawner.hpp"
#include "PlayerController.hpp"
#include "MonsterSpawner.hpp"
#include "MeshRenderer.hpp"
#include "HeartUI.hpp"
#include "HitEffect.hpp"
#include "BombCooldownUI.hpp"
#include "PlayerHealth.hpp"
#include "BombSpawner.hpp"
#include "TextUI.hpp"
#include "StatsUIUpdater.hpp"
#include "ResultUIUpdater.hpp"
#include "BlinkUIUpdater.hpp"
#include "AmmoUI.hpp"
#include "Background.hpp"
#include "ScreenShakeEffect.hpp"
#include "DeadMonsterController.hpp"
#include "LeaderboardUIUpdater.hpp"

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
    ShaderSet textureShader; // 텍스처 매핑용 셰이더(texture.hlsl)

    // 게임 내내 재사용하는 리소스 (Initialize에서 1회 생성)
    Mesh*          playerMesh     = nullptr;
    Mesh*          playerSpriteMesh = nullptr;
    Mesh*          bombMesh       = nullptr;
    Mesh*          bombSpriteMesh = nullptr;
    Mesh*          bombEffectSpriteMesh = nullptr;
    Mesh*          monsterMesh    = nullptr;
    Mesh*          playerBulletSpriteMesh = nullptr;
    Texture*       playerTexture  = nullptr;
    Texture*       playerBulletTexture = nullptr;
    Texture*       bombTexture = nullptr;
    Texture*       bombEffectTexture = nullptr;
    TextureMaterial* playerTextureMaterial = nullptr;
    TextureMaterial* playerBulletTextureMaterial = nullptr;
    ColorMaterial* playerMaterial = nullptr;
    ColorMaterial* playerBulletMaterial = nullptr;  // 탄환 전용 ColorMaterial
    ColorMaterial* monsterMaterial = nullptr;

    // UI 루트 GameObject (= 상태별 캔버스). Initialize에서 1회 생성, 소멸자에서 1회 정리.
    // 패널은 별도 클래스 없이 "끌 수 있는 GameObject"로 두고, 상태 루프가 해당 UI만 구동한다.
    GameObject* playingCanvas  = nullptr;
    GameObject* gameOverCanvas = nullptr;
    GameObject* lobbyCanvas    = nullptr;
    GameObject* background = nullptr;   // Playing 배경(텍스처) — world보다 먼저 렌더
    HitEffect*  hitEffect  = nullptr;   // 라운드마다 onDamaged 콜백 재바인딩용 핸들

    // Playing 진입 시 생성, GameOver 퇴장 시 world와 함께 삭제
    GameObject* player          = nullptr;
    GameObject* monsterSpawner  = nullptr;

    // screen 흔들리는 역할
    GameObject* screenShakeObject = nullptr;
    ScreenShakeEffect* screenShake = nullptr;   // 핸들 (screenShakeObject가 소유)

    // StatsUI 데이터 소스 — 주소를 StatsUI에 넘겨 매 프레임 읽게 한다.
    // 통계 데이터 소스 — 주소를 StatsUIUpdater/ResultUIUpdater에 넘겨 매 프레임 읽게 한다.
    // GameLoop 멤버라 주소가 안정적이며, OnEnter(Playing)에서 0으로 리셋한다.
    int   m_killCount = 0;     // 현재 라운드 누적 킬
    float m_playTime  = 0.0f;  // 현재 라운드 생존 시간(초)

    // 리더보드: GameOver 진입 시 점수 업로드 + TOP10 조회(워커 스레드).
    Leaderboard m_leaderboard;

    GameLoop() {
        LOG_DEBUG("GameLoop Created.");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        for (auto obj : pendingObjects) delete obj;
        pendingObjects.clear();
        delete playingCanvas;      playingCanvas      = nullptr;
        delete gameOverCanvas;     gameOverCanvas     = nullptr;
        delete lobbyCanvas;        lobbyCanvas        = nullptr;
        delete background;     background     = nullptr;
        delete playerMesh;     playerMesh     = nullptr;
        delete playerSpriteMesh; playerSpriteMesh = nullptr;
        delete playerBulletSpriteMesh; playerBulletSpriteMesh = nullptr;
        delete bombSpriteMesh; bombSpriteMesh = nullptr;
        delete bombEffectSpriteMesh; bombEffectSpriteMesh = nullptr;
        delete monsterMesh;    monsterMesh    = nullptr;
        delete playerTextureMaterial; playerTextureMaterial = nullptr;
        delete playerBulletTextureMaterial; playerBulletTextureMaterial = nullptr;
        delete playerTexture;  playerTexture  = nullptr;
        delete playerBulletTexture; playerBulletTexture = nullptr;
        delete bombTexture; bombTexture = nullptr;
        delete bombEffectTexture; bombEffectTexture = nullptr;
        delete playerMaterial; playerMaterial = nullptr;
        delete playerBulletMaterial; playerBulletMaterial = nullptr;
        delete monsterMaterial; monsterMaterial = nullptr;
        delete bombMesh;       bombMesh       = nullptr;
        delete screenShakeObject; screenShakeObject = nullptr;
        GraphicsContext::Destroy();
        shader.Release();
        textureShader.Release();
        CoUninitialize();
        LOG_DEBUG("GameLoop Destroyed.");
    }

    /// <summary>
    /// 클래스 초기화 함수, main문 시작 시 1회만 호출
    /// </summary>
    bool Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
                    int w = 1920, int h = 1080) {
        // WIC(텍스처 로딩)는 COM 기반이라 스레드에서 1회 초기화 필요
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        if (!win.Initialize(hInst, w, h, wndProc)) return false;

        if (!GraphicsContext::Create(win.hWnd, w, h)) return false;

        D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // 셰이더 컴파일 및 생성
        shader = GraphicsContext::Get()->CompileAndCreate(L"effect.hlsl", 0, true, ied, 2);

        // 텍스처 셰이더: POSITION(offset 0) + TEXCOORD(offset 28). COLOR(offset 12)는 건너뜀.
        D3D11_INPUT_ELEMENT_DESC tied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        textureShader = GraphicsContext::Get()->CompileAndCreate(L"texture.hlsl", 0, true, tied, 2);

        // 게임 내내 재사용하는 메시/머테리얼 리소스 1회 생성
        std::vector<Vertex> playerVertices =
            CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

        playerMesh = new Mesh();
        playerMesh->Create(playerVertices);

        playerSpriteMesh = new Mesh();
        playerSpriteMesh->Create(CreateTexturedQuad(1.2f, 1.2f));

        playerBulletSpriteMesh = new Mesh();
        playerBulletSpriteMesh->Create(CreateTexturedQuad(1.0f, 170.0f / 177.0f));

        std::vector<Vertex> bombVertices =
            CreateCircleVertices(1.0f, 64, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

        bombMesh = new Mesh();
        bombMesh->Create(bombVertices);

        bombSpriteMesh = new Mesh();
        bombSpriteMesh->Create(CreateTexturedQuad(1.0f, 1.0f));

        bombEffectSpriteMesh = new Mesh();
        bombEffectSpriteMesh->Create(CreateTexturedQuad(1.0f, 1.0f));

        monsterMesh = new Mesh();
        monsterMesh->Create(CreateTexturedQuad(1.0f, 1.0f));

        playerMaterial = new ColorMaterial(shader, XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f));

        playerTexture = new Texture();
        if (playerTexture->Load(GraphicsContext::Get()->Device, L"player.png"))
        {
            playerTexture->CreateSampler(GraphicsContext::Get()->Device);
            playerTextureMaterial = new TextureMaterial(textureShader, playerTexture);
        }
        else
        {
            LOG_WARN("Failed to load player texture. Fallback to color mesh.");
            delete playerTexture;
            playerTexture = nullptr;
        }

        playerBulletMaterial = new ColorMaterial(shader, XMFLOAT4(1.0f, 0.9f, 0.2f, 1.0f));

        playerBulletTexture = new Texture();
        if (playerBulletTexture->Load(GraphicsContext::Get()->Device, L"spit.png"))
        {
            playerBulletTexture->CreateSampler(GraphicsContext::Get()->Device);
            playerBulletTextureMaterial = new TextureMaterial(textureShader, playerBulletTexture);
        }
        else
        {
            LOG_WARN("Failed to load player bullet texture. Fallback to color mesh.");
            delete playerBulletTexture;
            playerBulletTexture = nullptr;
        }

        bombTexture = new Texture();
        if (bombTexture->Load(GraphicsContext::Get()->Device, L"bomb.png"))
        {
            bombTexture->CreateSampler(GraphicsContext::Get()->Device);
        }
        else
        {
            LOG_WARN("Failed to load bomb texture. Fallback to color mesh.");
            delete bombTexture;
            bombTexture = nullptr;
        }

        bombEffectTexture = new Texture();
        if (bombEffectTexture->Load(GraphicsContext::Get()->Device, L"bomb_effect.png"))
        {
            bombEffectTexture->CreateSampler(GraphicsContext::Get()->Device);
        }
        else
        {
            LOG_WARN("Failed to load bomb effect texture. Fallback to color mesh.");
            delete bombEffectTexture;
            bombEffectTexture = nullptr;
        }

        monsterMaterial = new ColorMaterial(shader, XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f));

        // ── UI 캔버스 1회 구성 ──────────────────────────────────────────────
        // 패널 = 끌 수 있는 GameObject. 요소 = Component. player는 이중 포인터(&player)로
        // 넘겨 라운드마다 재생성돼도 항상 현재 플레이어를 가리키게 한다.
        hitEffect = new HitEffect();

        playingCanvas = new GameObject(0.0f, 0.0f, 0.0f);
        playingCanvas->AddComponent(new HeartUI(shader, &player));
        playingCanvas->AddComponent(hitEffect);
        playingCanvas->AddComponent(new BombCooldownUI(shader, &player));
        // 상단 시간/킬: 라벨(TextUI)은 엔진이 렌더, 컨트롤러는 값만 갱신
        TextUI* timeLabel = new TextUI(shader, "00:00", -0.95f, 0.85f, 0.08f,
            XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), TextUI::Align::Left);
        TextUI* killLabel = new TextUI(shader, "KILL 0", 0.95f, 0.85f, 0.08f,
            XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f), TextUI::Align::Right);
        playingCanvas->AddComponent(timeLabel);
        playingCanvas->AddComponent(killLabel);
        playingCanvas->AddComponent(new StatsUIUpdater(&m_killCount, &m_playTime, timeLabel, killLabel));
        playingCanvas->AddComponent(new AmmoUI(shader, &player));

        // GameOver 캔버스: player==nullptr이라 빈 하트로 표시됨.
        // m_killCount/m_playTime은 다음 라운드 진입 전까지 최종값 유지 → 결과 표시에 사용.
        gameOverCanvas = new GameObject(0.0f, 0.0f, 0.0f);
        //gameOverUI->AddComponent(new HeartUI(shader, &player));
        gameOverCanvas->AddComponent(new TextUI(shader, "GAME OVER",
            0.0f, 0.40f, 0.22f, XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f)));     // 타이틀, 빨강
        // 최종 점수: 라벨(TextUI) + 값 갱신 컨트롤러
        TextUI* timeResult = new TextUI(shader, "TIME 00:00", 0.0f, 0.02f, 0.10f,
            XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), TextUI::Align::Center);
        TextUI* killResult = new TextUI(shader, "KILLS 0", 0.0f, -0.16f, 0.10f,
            XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f), TextUI::Align::Center);
        TextUI* scoreResult = new TextUI(shader, "SCORE 00000", 0.0f, -0.34f, 0.10f, 
            XMFLOAT4(0.30f, 0.90f, 1.00f, 1.0f), TextUI::Align::Center);
        gameOverCanvas->AddComponent(timeResult);
        gameOverCanvas->AddComponent(killResult);
        gameOverCanvas->AddComponent(scoreResult);
        gameOverCanvas->AddComponent(new ResultUIUpdater(&m_killCount, &m_playTime, timeResult, killResult, scoreResult));

        // ── 리더보드(TOP 10) UI ─────────────────────────────────────────────
        // 우측 컬럼에 헤딩 + 10개 행. 결과 텍스트는 LeaderboardUIUpdater가 매 프레임 갱신.
        gameOverCanvas->AddComponent(new TextUI(shader, "TOP 10",
            0.60f, 0.13f, 0.09f, XMFLOAT4(0.55f, 0.55f, 1.00f, 1.0f), TextUI::Align::Center));
        std::vector<TextUI*> lbRows;
        for (int i = 0; i < 10; ++i) {
            float y = 0.0f - i * 0.085f;
            TextUI* row = new TextUI(shader, "", 0.40f, y, 0.05f,
                XMFLOAT4(0.90f, 0.90f, 0.90f, 1.0f), TextUI::Align::Left);
            gameOverCanvas->AddComponent(row);
            lbRows.push_back(row);
        }
        gameOverCanvas->AddComponent(new LeaderboardUIUpdater(&m_leaderboard, lbRows));

        gameOverCanvas->AddComponent(new TextUI(shader, "SPACE:RESTART  1:LOBBY",
            0.0f, -0.73f, 0.06f, XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f)));    // 안내, 회색

        // Lobby 캔버스: 타이틀 + 태그라인 + 깜빡이는 시작 안내 + 조작 안내
        lobbyCanvas = new GameObject(0.0f, 0.0f, 0.0f);
        lobbyCanvas->AddComponent(new TextUI(shader, "SPIT MASTER",
            0.0f, 0.45f, 0.20f, XMFLOAT4(0.30f, 0.90f, 1.00f, 1.0f)));     // 타이틀, 시안
        lobbyCanvas->AddComponent(new TextUI(shader, "SPIT TO SURVIVE",
            0.0f, 0.18f, 0.06f, XMFLOAT4(0.80f, 0.80f, 0.80f, 1.0f)));     // 태그라인
        // 깜빡이는 시작 안내: 라벨(TextUI) + 색 펄스 컨트롤러
        XMFLOAT4 pressColor = XMFLOAT4(1.00f, 0.90f, 0.20f, 1.0f);
        TextUI* pressLabel = new TextUI(shader, "PRESS SPACE",
            0.0f, -0.15f, 0.09f, pressColor, TextUI::Align::Center);
        lobbyCanvas->AddComponent(pressLabel);
        lobbyCanvas->AddComponent(new BlinkUIUpdater(pressLabel, pressColor, 4.5f));
        lobbyCanvas->AddComponent(new TextUI(shader, "SPACE:START  ESC:QUIT",
            0.0f, -0.72f, 0.05f, XMFLOAT4(0.55f, 0.55f, 0.55f, 1.0f)));    // 조작 안내, 회색

        // 배경(텍스처 매핑 테스트): 장판 타일 이미지를 화면 전체에 깔기
        background = new GameObject(0.0f, 0.0f, 0.0f);
        background->AddComponent(new Background(textureShader, L"bg.png"));

        // 스크린 흔들리는 오브젝트
        screenShakeObject = new GameObject(0.0f, 0.0f, 0.0f);
        screenShake = new ScreenShakeEffect();
        screenShakeObject->AddComponent(screenShake);

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

            // 라운드 통계 리셋 (StatsUIUpdater가 이 멤버들의 주소를 읽고 있음)
            m_killCount = 0;
            m_playTime  = 0.0f;

            // 플레이어 오브젝트 생성
            player = new GameObject(0.0f, 0.0f, 0.0f);
            bool usePlayerTexture = playerSpriteMesh && playerTextureMaterial;
            float playerScale = usePlayerTexture ? 0.272f : 0.08f;
            float playerColliderRadius = usePlayerTexture ? 0.39f : 1.0f;
            float playerRotationOffset = usePlayerTexture ? -XM_PIDIV2 : 0.0f;
            float playerScreenMargin = playerScale * playerColliderRadius;

            player->scale = { playerScale, playerScale, 1.0f };

            if (usePlayerTexture)
                player->AddComponent(new MeshRenderer(playerSpriteMesh, playerTextureMaterial));
            else
                player->AddComponent(new MeshRenderer(playerMesh, playerMaterial));

            player->AddComponent(new PlayerController(
                win.hWnd, &win.Width, &win.Height,
                playerRotationOffset, playerScreenMargin
            ));
            player->AddComponent(new PlayerHealth(3, 1.5f));
            bool usePlayerBulletTexture = playerBulletSpriteMesh && playerBulletTextureMaterial;
            player->AddComponent(new PlayerBulletSpawner(
                &pendingObjects,
                usePlayerBulletTexture ? playerBulletSpriteMesh : playerMesh,
                usePlayerBulletTexture ? static_cast<Material*>(playerBulletTextureMaterial) : static_cast<Material*>(playerBulletMaterial),
                win.hWnd, &win.Width, &win.Height,
                usePlayerBulletTexture ? 0.0825f : 0.03f,
                usePlayerBulletTexture ? 0.6f : 1.0f,
                0.0f
            ));
            bool useBombTexture = bombSpriteMesh && bombEffectSpriteMesh && bombTexture && bombEffectTexture;
            player->AddComponent(new BombSpawner(
                &world, &pendingObjects,
                useBombTexture ? bombSpriteMesh : bombMesh,
                shader, textureShader,
                useBombTexture ? bombTexture : nullptr,
                useBombTexture ? bombEffectTexture : nullptr,
                useBombTexture ? bombEffectSpriteMesh : bombMesh,
                screenShake
            ));
            player->AddComponent(new CircleCollider(playerColliderRadius, CollisionLayer::Player));

            // 몬스터 스포너 오브젝트 생성
            monsterSpawner = new GameObject(100, 100, 100);
            monsterSpawner->AddComponent(new MonsterSpawner(&pendingObjects, player, textureShader,monsterMesh));

            world.push_back(player);
            world.push_back(monsterSpawner);

            // UI는 Initialize에서 이미 구성됨. 새 player의 Health 콜백만 재바인딩.
            PlayerHealth* health = player->GetComponent<PlayerHealth>();
            if (health)
            {
                health->onDamaged = [this]() { hitEffect->Trigger(); };
            }

            // 이전 라운드 사망 직전 Trigger()로 남은 피격 이펙트 페이드 값 초기화.
            // (없으면 재시작 첫 프레임에 빨간 이펙트가 한 번 번쩍임)
            if (hitEffect) hitEffect->Reset();

            break;
        }
        case State::GameOver:
            LOG_DEBUG("State Enter: GameOver");
            LOG_INFO("Game Over! Press SPACE to restart / 1 to lobby");

            // 점수 계산(ResultUIUpdater와 동일 공식) 후 리더보드 업로드+조회 시작
            {
                int finalScore = 10 * (int)m_playTime + 100 * m_killCount;
                m_leaderboard.Submit(finalScore);
            }
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
            // Playing 종료 시 라운드 오브젝트를 정리한다. UI 캔버스와 최종 점수는 유지된다.
            for (auto obj : pendingObjects) delete obj;
            pendingObjects.clear();
            for (auto obj : world) delete obj;
            world.clear();
            player         = nullptr;
            monsterSpawner = nullptr;
            break;
        case State::GameOver:
            LOG_DEBUG("State Exit: GameOver");
            m_leaderboard.Join();   // 워커 스레드 정리(다음 라운드/로비 진입 전)
            break;
        }
    }

    void Input() {
        // F: 전체화면(borderless) <-> 1920x1080 창모드 토글 (모든 상태 공통)
        if (GetAsyncKeyState('F') & 0x0001) {
            win.ToggleFullscreen();
            GraphicsContext::Get()->Resize(win.Width, win.Height);
        }

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
            lobbyCanvas->Update(dt);
            break;
        case State::Playing:
            m_playTime += dt; // 생존 시간 누적
            background->Update(dt); // 첫 프레임 Start() 트리거(텍스처 로드)

            // 생성 예약된 오브젝트를 world로 push_back
            for (auto obj : pendingObjects) world.push_back(obj);
            pendingObjects.clear();

            for (auto obj : world) obj->Update(dt);
            playingCanvas->Update(dt);

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
                    // 몬스터가 죽어서 제거되는 경우 → 킬 1 증가.
                    // 몬스터는 불릿/폭탄 모두 getDamaged()→DEAD→isObjDead 경로로만
                    // 죽으므로, 제거 시점에서 세면 사망 원인과 무관하게 정확히 카운트된다.
                    if ((*obj)->GetComponent<MeleeMonsterController>() ||
                        (*obj)->GetComponent<RangedMonsterController>()) {
                        m_killCount++;
                        MonsterSpawner* curMonsterSpawner = monsterSpawner->GetComponent<MonsterSpawner>();
                        curMonsterSpawner->setSpawnedCount(curMonsterSpawner->getSpawnedCount() - 1); // 몬스터 죽어서 스폰된 몬스터 개수 감소
                        GameObject* deadMonster = new GameObject((*obj)->pos.x, (*obj)->pos.y, (*obj)->pos.z);
                        TextureMaterial* curMonsterMat;
                        if ((*obj)->GetComponent<MeleeMonsterController>()) {
                            curMonsterMat = curMonsterSpawner->deadMonsterMesh(0);
                        }
                        else {
                            curMonsterMat = curMonsterSpawner->deadMonsterMesh(1);
                        }
                        deadMonster->AddComponent(new DeadMonsterController(player, 0.5f));
                        deadMonster->AddComponent(new MeshRenderer(monsterMesh, curMonsterMat));
                        deadMonster->scale = { 0.1f, 0.1f, 1.0f };

                        pendingObjects.push_back(deadMonster);
                    }
                    delete* obj;
                    obj = world.erase(obj);
                    continue;
                }
                obj++;

                continue;
            }

            if (screenShakeObject)
                screenShakeObject->Update(dt);

            break;
        case State::GameOver:
            gameOverCanvas->Update(dt);
            break;
        }
    }

    void Render() {
        auto* gfx = GraphicsContext::Get();
        switch (m_state) {
        case State::Lobby:
        {
            float col[] = { 0.06f, 0.07f, 0.10f, 1.0f };   // 어두운 배경
            gfx->ImmediateContext->ClearRenderTargetView(gfx->RTV, col);

            D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
            gfx->ImmediateContext->RSSetViewports(1, &vp);
            gfx->ImmediateContext->OMSetRenderTargets(1, &gfx->RTV, nullptr);
            gfx->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            lobbyCanvas->Render();
            break;
        }
        case State::Playing:
        {
            float col[] = { 0.1f, 0.2f, 0.3f, 1.0f };
            gfx->ImmediateContext->ClearRenderTargetView(gfx->RTV, col);

            XMFLOAT2 screenShakeOffset = screenShake->GetPixelOffset((float)win.Width, (float)win.Height);
            D3D11_VIEWPORT vp = { screenShakeOffset.x, screenShakeOffset.y, (float)win.Width, (float)win.Height, 0, 1 };
            gfx->ImmediateContext->RSSetViewports(1, &vp);
            gfx->ImmediateContext->OMSetRenderTargets(1, &gfx->RTV, nullptr);
            gfx->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            float blendFactor[4] = { 0, 0, 0, 0 };
            gfx->ImmediateContext->OMSetBlendState(gfx->AlphaBlendState, blendFactor, 0xffffffff);

            background->Render();  // 배경을 가장 먼저(가장 뒤에) 그림
            for (auto obj : world) obj->Render();
            playingCanvas->Render();  // UI는 항상 게임 오브젝트 위에 렌더링
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

            gameOverCanvas->Render();  // player==null → 빈 하트 (0개 상태로 표시)
            break;
        }
        }
        gfx->SwapChain->Present(gfx->VSync, 0);
    }
};
