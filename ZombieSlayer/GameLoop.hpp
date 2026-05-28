#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"
#include "Logger.hpp"

class GameLoop {
public:
    enum class State { Lobby, Playing, GameOver };

    WindowContext win;
    DeltaTime     timer;
    std::vector<GameObject*> world;

    GameLoop() {
        LOG_INFO("GameLoop Created.");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        GraphicsContext::Destroy();
        LOG_INFO("GameLoop Destroyed.");
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
        LOG_INFO("Game loop started");
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
        LOG_INFO("Game loop ended");
    }

private:
    State m_state     = State::Lobby;
    bool  m_isRunning = true;

    void OnEnter(State s) {
        switch (s) {
        case State::Lobby:    LOG_INFO("State Enter: Lobby");    break;
        case State::Playing:  LOG_INFO("State Enter: Playing");  break;
        case State::GameOver: LOG_INFO("State Enter: GameOver"); break;
        }
    }

    void OnExit(State s) {
        switch (s) {
        case State::Lobby:    LOG_INFO("State Exit: Lobby");    break;
        case State::Playing:  LOG_INFO("State Exit: Playing");  break;
        case State::GameOver: LOG_INFO("State Exit: GameOver"); break;
        }
    }

    void Input() {
        switch (m_state) {
        case State::Lobby:

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

            break;
        case State::GameOver:

            break;
        }
        gfx->SwapChain->Present(gfx->VSync, 0);
    }
};
