#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "GameLoop.hpp"
#include "MeshRenderer.hpp"
#include "PlayerControl.hpp"

LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int) {
    GameLoop gEngine;
    if (!gEngine.Initialize(hI, GlobalWndProc)) {
        printf("[Error] Engine initialization failed.\n");
        return -1;
    }

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ShaderSet starShaders = GraphicsContext::Get()->CompileAndCreate(L"effect.hlsl", 0, true, ied, 2);

    gEngine.Run();

    starShaders.Release();
    return 0;
}
