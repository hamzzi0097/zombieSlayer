#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"

class GameLoop {
public:
    WindowContext win;
    DeltaTime     timer;
    std::vector<GameObject*> world;
    bool isRunning = true;

    GameLoop() {
        printf("[Engine] GameLoop Created.\n");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        GraphicsContext::Destroy();
        printf("[Engine] GameLoop Destroyed.\n");
    }

    bool Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
                    int w = 800, int h = 600) {
        if (!win.Initialize(hInst, w, h, wndProc)) return false;
        return GraphicsContext::Create(win.hWnd, w, h);
    }

    void Input() {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        // Resize window (C key)
        if (GetAsyncKeyState('C') & 0x0001) {
            win.Width = 600; win.Height = 600;
            RECT rc = { 0, 0, win.Width, win.Height };
            AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
            SetWindowPos(win.hWnd, nullptr, 0, 0,
                         rc.right - rc.left, rc.bottom - rc.top,
                         SWP_NOMOVE | SWP_NOZORDER);
            GraphicsContext::Get()->Resize(win.Width, win.Height);
        }

        for (auto obj : world) obj->Input();
    }

    void Update() {
        float dt = timer.GetDelta();
        for (auto obj : world) obj->Update(dt, GraphicsContext::Get());
    }

    void Render() {
        auto* gfx = GraphicsContext::Get();
        float col[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        gfx->ImmediateContext->ClearRenderTargetView(gfx->RTV, col);

        D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
        gfx->ImmediateContext->RSSetViewports(1, &vp);
        gfx->ImmediateContext->OMSetRenderTargets(1, &gfx->RTV, nullptr);
        gfx->ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (auto obj : world) obj->Render(gfx);

        gfx->SwapChain->Present(gfx->VSync, 0);
    }

    void Run() {
        MSG msg = {};
        while (msg.message != WM_QUIT && isRunning) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                Input();
                Update();
                Render();
            }
        }
    }
};
