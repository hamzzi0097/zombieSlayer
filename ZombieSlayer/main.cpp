#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "GameLoop.hpp"
#include "MeshRenderer.hpp"
#include "PlayerControl.hpp"
#include "PlayerBullet.hpp"
#include "PlayerBulletSpawner.hpp"
#include "Logger.hpp"
#include "MonsterSpawner.hpp"
#include "Collider.hpp"
#include "MonsterBulletSpawner.hpp"

// GraphicsContext 싱글톤 인스턴스 정의
GraphicsContext* GraphicsContext::s_instance = nullptr;

// -----------------------------------------------------------------------------
// [윈도우 메시지 처리기]
// -----------------------------------------------------------------------------
LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

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

// -----------------------------------------------------------------------------
// [메인 엔트리 포인트]
// -----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS)
{
    // 엔진 매니저 초기화
    GameLoop gEngine;
    if (!gEngine.Initialize(hI, GlobalWndProc)) {
        LOG_ERROR("Engine initialization failed.");
        return -1;
    }

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // 셰이더 컴파일 및 생성
    ShaderSet starShaders = GraphicsContext::Get()->CompileAndCreate(L"effect.hlsl", 0, true, ied, 2);
  
    // 플레이어 원 Mesh 생성
    std::vector<Vertex> playerVertices =
        CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    Mesh playerMesh;
    playerMesh.Create(playerVertices);

    ColorMaterial playerMaterial(
        starShaders,
        XMFLOAT4(0.2f, 0.8f, 1.0f, 1.0f)
    );

 
    // 플레이어 오브젝트 구성
    GameObject* player = new GameObject(0.0f, 0.0f, 0.0f);
    player->scale = { 0.05f, 0.05f, 1.0f };

    player->AddComponent(new MeshRenderer(&playerMesh, &playerMaterial));   // 렌더링 컴포넌트 추가
    player->AddComponent(new PlayerController(gEngine.win.hWnd, &gEngine.win.Width, &gEngine.win.Height));  // 이동/회전 컴포넌트 추가
    player->AddComponent(new PlayerBulletSpawner(   //탄환 스포너 컴포넌트 추가
        &gEngine.pendingObjects,
        &playerMesh,
        &playerMaterial,
        gEngine.win.hWnd,
        &gEngine.win.Width,
        &gEngine.win.Height
    ));
    player->AddComponent(new CircleCollider(1.0f, CollisionLayer::Player));

    gEngine.world.push_back(player);

    GameObject* mon = new GameObject(100, 100, 100);
    mon->AddComponent(new MonsterSpawner(&gEngine.pendingObjects, player, starShaders, CreateCircleVertices(1.0f, 256, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f))));
    
    gEngine.world.push_back(mon);
    LOG_DEBUG("GameLoop Start!");
    gEngine.Run();

    // 메모리 해제
    starShaders.Release();

    return 0;
}
