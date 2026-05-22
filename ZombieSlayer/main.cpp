#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "GameLoop.hpp"
#include "MeshRenderer.hpp"
#include "PlayerControl.hpp"
#include "PlayerBullet.hpp"
#include "PlayerBulletSpawner.hpp"
#include "Logger.hpp"

GraphicsContext* GraphicsContext::s_instance = nullptr;

LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

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


int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS)
{
    GameLoop gEngine;
    if (!gEngine.Initialize(hI, GlobalWndProc)) {
        LOG_ERROR("Engine initialization failed.");
        return -1;
    }

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ShaderSet starShaders = GraphicsContext::Get()->CompileAndCreate(L"effect.hlsl", 0, true, ied, 2);
  
    std::vector<Vertex> playerVertices =
        CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    Mesh playerMesh;
    playerMesh.Create(playerVertices);

    ColorMaterial playerMaterial(
        starShaders,
        XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f)
    );

    // player test
    GameObject* player = new GameObject(0.0f, 0.0f, 0.0f);
    player->scale = { 0.08f, 0.08f, 1.0f };

    player->AddComponent(new MeshRenderer(&playerMesh, &playerMaterial));
    player->AddComponent(new PlayerController(gEngine.win.hWnd, &gEngine.win.Width, &gEngine.win.Height));
    player->AddComponent(new PlayerBulletSpawner(
        &gEngine.world,
        &playerMesh,
        &playerMaterial,
        gEngine.win.hWnd,
        &gEngine.win.Width,
        &gEngine.win.Height
    ));

    gEngine.world.push_back(player);

    // playerbullet test
    GameObject* testBullet = new GameObject(-0.5f, 0.0f, 0.0f);
    testBullet->scale = { 0.05f, 0.05f, 1.0f };

    testBullet->AddComponent(new MeshRenderer(&playerMesh, &playerMaterial));
    testBullet->AddComponent(new PlayerBullet({ 1.0f, 0.0f }, 0.5f, 2.0f));

    gEngine.world.push_back(testBullet);
    
    LOG_INFO("GameLoop Start!");
    gEngine.Run();

    starShaders.Release();
    return 0;
}
