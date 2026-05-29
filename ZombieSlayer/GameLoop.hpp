#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"
#include "Logger.hpp"
#include "Collider.hpp"

class GameLoop {
public:
    enum class State { Lobby, Playing, GameOver };

    WindowContext win;
    DeltaTime timer;
    std::vector<GameObject*> world;
    std::vector<GameObject*> pendingObjects;

    GameLoop() {
        LOG_DEBUG("GameLoop Created.");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        pendingObjects.clear();
        GraphicsContext::Destroy();
        LOG_DEBUG("GameLoop Destroyed.");
    }

    bool Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
                    int w = 800, int h = 600) {
        if (!win.Initialize(hInst, w, h, wndProc)) return false;
        return GraphicsContext::Create(win.hWnd, w, h);
    }

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

    void OnEnter(State s) {
        switch (s) {
        case State::Lobby:
            LOG_DEBUG("State Enter: Lobby");
            LOG_INFO("=== ZombieSlayer ===");
            LOG_INFO("Press SPACE to start");
            break;
        case State::Playing:
            LOG_DEBUG("State Enter: Playing");
            LOG_INFO("Game Start! Survive as long as you can.");
            break;
        case State::GameOver:
            LOG_DEBUG("State Enter: GameOver");
            LOG_INFO("Game Over! Press ENTER to restart / ESC to lobby");
            break;
        }
    }

    void OnExit(State s) {
        switch (s) {
        case State::Lobby:    LOG_DEBUG("State Exit: Lobby");    break;
        case State::Playing:  LOG_DEBUG("State Exit: Playing");  break;
        case State::GameOver: LOG_DEBUG("State Exit: GameOver"); break;
        }
    }

    void Input() {
        for (auto obj : world) obj->Input();

        switch (m_state) {
        case State::Lobby:
            if (GetAsyncKeyState(VK_SPACE) & 0x0001)
                ChangeState(State::Playing);
            break;
        case State::Playing:

            break;
        case State::GameOver:

            break;
        }
    }

    void Update(float dt) {
        switch (m_state) {
        case State::Lobby:

            break;
        case State::Playing:
            // 생성 예약된 오브젝트를 world로 push_back
            for (auto obj : pendingObjects) world.push_back(obj);
            pendingObjects.clear();

            for (auto obj : world) obj->Update(dt);

            // 이동 업데이트 이후 죽은 오브젝트 제거 전 충돌 검사
            CheckOnCollisions();

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

            for (auto obj : world) obj->Render();
            break;
        }
        case State::GameOver:


            break;
        }
        gfx->SwapChain->Present(gfx->VSync, 0);
    }
};
